/**
 * Times the same StorageAccess calls twice: directly against PersistentStorage, and against a
 * running sourcetrail_engine over HTTP+JSON. The delta between the two columns is what the
 * client/engine boundary costs per query.
 *
 * Usage:
 *   Sourcetrail_bench --db <path>.srctrldb [--endpoint host:port --token <t>] [--iterations N]
 *
 * Without an endpoint only the direct column is measured. Spawning the engine and reading its
 * "ENGINE_PORT <port> <token>" handshake belongs to scripts/bench_queries.py, which already has it.
 */
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <fmt/core.h>

#include <spdlog/sinks/sink.h>
#include <spdlog/spdlog.h>

#include "data/graph/Graph.h"
#include "data/location/SourceLocationCollection.h"
#include "data/location/SourceLocationFile.h"
#include "data/NodeTypeSet.h"
#include "data/storage/PersistentStorage.h"
#include "data/storage/StorageStats.h"
#include "data/tooltip/TooltipOrigin.h"
#include "EngineChannel.h"
#include "FilePath.h"
#include "HttpStorageAccess.h"
#include "MessageQueue.h"
#include "settings/details/ApplicationSettings.h"
#include "settings/IApplicationSettings.hpp"

namespace {

using Clock = std::chrono::steady_clock;
using Millis = std::chrono::duration<double, std::milli>;

struct Options {
  std::string dbPath;
  std::string endpoint;
  std::string token;
  int iterations = 20;
};

struct Sample {
  double median = 0.0;
  double p95 = 0.0;
  size_t resultSize = 0;
};

// Percentiles off a sorted copy; N is small, so simplicity beats nth_element here.
Sample summarize(std::vector<double> timings, size_t resultSize) {
  std::sort(timings.begin(), timings.end());
  Sample sample;
  sample.median = timings[timings.size() / 2];
  sample.p95 = timings[static_cast<size_t>(static_cast<double>(timings.size() - 1) * 0.95)];
  sample.resultSize = resultSize;
  return sample;
}

/** Runs `call` `iterations` times after one warm-up, which is discarded. */
Sample measure(int iterations, const std::function<size_t()>& call) {
  size_t resultSize = call();
  std::vector<double> timings;
  timings.reserve(static_cast<size_t>(iterations));
  for(int i = 0; i < iterations; ++i) {
    const auto start = Clock::now();
    resultSize = call();
    timings.push_back(Millis(Clock::now() - start).count());
  }
  return summarize(std::move(timings), resultSize);
}

size_t graphSize(const std::shared_ptr<Graph>& graph) {
  return graph ? graph->getNodeCount() + graph->getEdgeCount() : 0;
}

/**
 * The call set, in payload-size order. Every entry runs against whichever StorageAccess it is
 * handed, so the two columns are guaranteed to be the same work.
 */
struct Query {
  std::string name;
  std::function<size_t(const StorageAccess&)> run;
};

std::vector<Query> makeQueries(const StorageAccess& reference) {
  // Pick real inputs out of the index rather than inventing ids: an id that resolves to nothing
  // would time the error path instead of the query.
  std::vector<Id> tokenIds;
  for(const auto& match : reference.getAutocompletionMatches(L"a", NodeTypeSet::all(), false)) {
    if(!match.tokenIds.empty()) {
      tokenIds.push_back(match.tokenIds.front());
    }
    if(tokenIds.size() >= 5) {
      break;
    }
  }

  // A full-text hit is the cheapest way to get a path that is definitely in the index; a file node's
  // name off the graph is a NameHierarchy rendering and does not always round-trip to a FilePath.
  FilePath filePath;
  if(const auto locations = reference.getFullTextSearchLocations(L"storage", false); locations) {
    locations->forEachSourceLocationFile([&filePath](std::shared_ptr<SourceLocationFile> file) {
      if(filePath.empty() && file) {
        filePath = file->getFilePath();
      }
    });
  }

  std::vector<Query> queries;
  queries.push_back(
      {"getStorageStats", [](const StorageAccess& access) { return static_cast<size_t>(access.getStorageStats().nodeCount); }});
  queries.push_back({"getGraphForAll", [](const StorageAccess& access) { return graphSize(access.getGraphForAll()); }});
  if(!tokenIds.empty()) {
    queries.push_back({"getGraphForActiveTokenIds", [tokenIds](const StorageAccess& access) {
                         return graphSize(access.getGraphForActiveTokenIds(tokenIds, {}));
                       }});
    queries.push_back({"getTooltipInfoForTokenIds", [tokenIds](const StorageAccess& access) {
                         return access.getTooltipInfoForTokenIds(tokenIds, TooltipOrigin::TOOLTIP_ORIGIN_CODE).snippets.size();
                       }});
  }
  if(!filePath.empty()) {
    queries.push_back({"getSourceLocationsForFile", [filePath](const StorageAccess& access) {
                         const auto locations = access.getSourceLocationsForFile(filePath);
                         return locations ? locations->getSourceLocationCount() : 0;
                       }});
  }
  queries.push_back({"getAutocompletionMatches", [](const StorageAccess& access) {
                       return access.getAutocompletionMatches(L"get", NodeTypeSet::all(), false).size();
                     }});
  queries.push_back({"getFullTextSearchLocations", [](const StorageAccess& access) {
                       const auto locations = access.getFullTextSearchLocations(L"storage", false);
                       return locations ? locations->getSourceLocationCount() : 0;
                     }});
  return queries;
}

}    // namespace

int main(int argc, char* argv[]) {
  if(auto* logger = spdlog::default_logger_raw(); logger != nullptr) {
    for(auto& sink : logger->sinks()) {
      sink->set_level(spdlog::level::level_enum::off);
    }
  }

  // PersistentStorage reads the text encoding out of the settings singleton and dispatches status
  // messages while it works; without both of these a full-text search dereferences null.
  IApplicationSettings::setInstance(std::make_shared<ApplicationSettings>());
  IMessageQueue::setInstance(std::make_shared<details::MessageQueue>());

  Options options;
  for(int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    const auto next = [&]() { return i + 1 < argc ? std::string(argv[++i]) : std::string(); };
    if(arg == "--db") {
      options.dbPath = next();
    } else if(arg == "--endpoint") {
      options.endpoint = next();
    } else if(arg == "--token") {
      options.token = next();
    } else if(arg == "--iterations") {
      options.iterations = std::atoi(next().c_str());
    }
  }

  if(options.dbPath.empty()) {
    fmt::println(stderr, "usage: Sourcetrail_bench --db <path>.srctrldb [--endpoint host:port --token <t>] [--iterations N]");
    return EXIT_FAILURE;
  }

  const FilePath dbPath(options.dbPath);
  const FilePath bookmarkPath(dbPath.getParentDirectory().concatenate(dbPath.withoutExtension().fileName() + L".srctrlbm"));

  PersistentStorage direct(dbPath, bookmarkPath);
  direct.setup();
  direct.setMode(SqliteIndexStorage::STORAGE_MODE_READ);
  direct.buildCaches();

  const auto queries = makeQueries(direct);

  // Spawning the engine is left to the caller (scripts/bench_queries.py reuses the integration
  // harness for it), so this only needs the endpoint and token off the handshake line.
  std::unique_ptr<EngineChannel> channel;
  std::unique_ptr<HttpStorageAccess> overHttp;
  if(!options.endpoint.empty()) {
    channel = std::make_unique<EngineChannel>(options.endpoint, options.token);
    if(!channel->waitUntilReady(std::chrono::seconds(30))) {
      fmt::println(stderr, "engine at {} never answered", options.endpoint);
      return EXIT_FAILURE;
    }
    overHttp = std::make_unique<HttpStorageAccess>(channel.get());
  }

  fmt::println("db: {}   iterations: {}", options.dbPath, options.iterations);
  fmt::println("{:<28} {:>11} {:>11} {:>11} {:>11} {:>8} {:>12}",
               "query",
               "direct ms",
               "direct p95",
               "http ms",
               "http p95",
               "factor",
               "result");
  for(const auto& query : queries) {
    const auto directSample = measure(options.iterations, [&] { return query.run(direct); });
    if(!overHttp) {
      fmt::println("{:<28} {:>11.3f} {:>11.3f} {:>11} {:>11} {:>8} {:>12}",
                   query.name,
                   directSample.median,
                   directSample.p95,
                   "-",
                   "-",
                   "-",
                   directSample.resultSize);
      continue;
    }
    const auto httpSample = measure(options.iterations, [&] { return query.run(*overHttp); });
    // A mismatched result size means the two paths did not do the same work -- the timings are then
    // not comparable, so say so rather than printing a factor that means nothing.
    const std::string result = httpSample.resultSize == directSample.resultSize ?
        std::to_string(directSample.resultSize) :
        fmt::format("{} != {}", directSample.resultSize, httpSample.resultSize);
    fmt::println("{:<28} {:>11.3f} {:>11.3f} {:>11.3f} {:>11.3f} {:>8.1f} {:>12}",
                 query.name,
                 directSample.median,
                 directSample.p95,
                 httpSample.median,
                 httpSample.p95,
                 directSample.median > 0.0 ? httpSample.median / directSample.median : 0.0,
                 result);
  }

  return EXIT_SUCCESS;
}
