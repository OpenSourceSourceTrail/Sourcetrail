#include "HttpServer.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <optional>
#include <set>
#include <thread>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <condition_variable>

#include "logging.h"

namespace beast = boost::beast;
namespace asio = boost::asio;
namespace bhttp = boost::beast::http;
using tcp = boost::asio::ip::tcp;

namespace http {

namespace {

/** Percent-decoding, plus '+' as a space in query values. */
std::string urlDecode(std::string_view text) {
  std::string out;
  out.reserve(text.size());
  for(size_t i = 0; i < text.size(); ++i) {
    if(text[i] == '%' && i + 2 < text.size()) {
      const auto hex = [](char chr) -> int {
        if(chr >= '0' && chr <= '9') {
          return chr - '0';
        }
        if(chr >= 'a' && chr <= 'f') {
          return chr - 'a' + 10;
        }
        if(chr >= 'A' && chr <= 'F') {
          return chr - 'A' + 10;
        }
        return -1;
      };
      const int high = hex(text[i + 1]);
      const int low = hex(text[i + 2]);
      if(high >= 0 && low >= 0) {
        out.push_back(static_cast<char>((high << 4) | low));
        i += 2;
        continue;
      }
    }
    out.push_back(text[i] == '+' ? ' ' : text[i]);
  }
  return out;
}

void parseTarget(std::string_view target, Request& request) {
  const size_t mark = target.find('?');
  request.path = urlDecode(target.substr(0, mark));
  if(mark == std::string_view::npos) {
    return;
  }

  std::string_view rest = target.substr(mark + 1);
  while(!rest.empty()) {
    const size_t amp = rest.find('&');
    const std::string_view pair = rest.substr(0, amp);
    if(const size_t eq = pair.find('='); eq != std::string_view::npos) {
      request.query.emplace(urlDecode(pair.substr(0, eq)), urlDecode(pair.substr(eq + 1)));
    } else if(!pair.empty()) {
      request.query.emplace(urlDecode(pair), std::string{});
    }
    if(amp == std::string_view::npos) {
      break;
    }
    rest = rest.substr(amp + 1);
  }
}

/** One connected server-sent-event reader. Broadcast enqueues; the connection's own thread writes. */
struct EventConnection {
  std::mutex mutex;
  std::condition_variable signal;
  std::deque<std::string> pending;
  bool closed{false};

  void push(std::string frame) {
    {
      const std::lock_guard<std::mutex> lock(mutex);
      if(closed) {
        return;
      }
      pending.push_back(std::move(frame));
    }
    signal.notify_one();
  }

  void close() {
    {
      const std::lock_guard<std::mutex> lock(mutex);
      closed = true;
    }
    signal.notify_all();
  }
};

}    // namespace

std::string Request::get(const std::string& key, const std::string& fallback) const {
  const auto found = query.find(key);
  return found != query.end() ? found->second : fallback;
}

uint64_t Request::getUInt(const std::string& key, uint64_t fallback) const {
  const auto found = query.find(key);
  if(found == query.end() || found->second.empty()) {
    return fallback;
  }
  try {
    return std::stoull(found->second);
  } catch(...) {
    return fallback;
  }
}

bool Request::getBool(const std::string& key, bool fallback) const {
  const auto found = query.find(key);
  if(found == query.end()) {
    return fallback;
  }
  return found->second == "true" || found->second == "1" || found->second.empty();
}

std::vector<std::string> Request::getList(const std::string& key) const {
  const auto found = query.find(key);
  if(found == query.end() || found->second.empty()) {
    return {};
  }

  std::vector<std::string> parts;
  std::string_view rest = found->second;
  while(!rest.empty()) {
    const size_t comma = rest.find(',');
    if(const std::string_view part = rest.substr(0, comma); !part.empty()) {
      parts.emplace_back(part);
    }
    if(comma == std::string_view::npos) {
      break;
    }
    rest = rest.substr(comma + 1);
  }
  return parts;
}

Response Response::json(std::string body) {
  return Response{200, std::move(body), "application/json"};
}

Response Response::error(int status, std::string_view message) {
  std::string body = R"({"error":")";
  for(const char chr : message) {
    if(chr == '"' || chr == '\\') {
      body.push_back('\\');
    }
    body.push_back(chr);
  }
  body += R"("})";
  return Response{status, std::move(body), "application/json"};
}

struct Server::Impl {
  explicit Impl(std::string token) : authToken(std::move(token)) {}

  std::string authToken;
  std::set<std::string> allowedOrigins;
  std::map<std::pair<std::string, std::string>, Handler> exactRoutes;
  std::map<std::pair<std::string, std::string>, Handler> prefixRoutes;
  std::string eventPath;

  asio::io_context io;
  std::unique_ptr<tcp::acceptor> acceptor;
  std::thread acceptThread;
  std::atomic<bool> stopping{false};
  uint16_t boundPort{0};

  std::mutex connectionsMutex;
  std::set<std::shared_ptr<EventConnection>> connections;
  // Sockets currently being served. stop() shuts them down so the threads blocked reading them
  // return instead of lingering until the peer happens to disconnect.
  std::set<tcp::socket*> activeSockets;

  /** Finds an exact route first, then the longest matching prefix route. */
  const Handler* findRoute(const std::string& method, const std::string& path, std::string& param) const {
    if(const auto found = exactRoutes.find({method, path}); found != exactRoutes.end()) {
      return &found->second;
    }

    const Handler* best = nullptr;
    size_t bestLength = 0;
    for(const auto& [key, handler] : prefixRoutes) {
      if(key.first != method || path.rfind(key.second, 0) != 0) {
        continue;
      }
      if(key.second.size() >= bestLength) {
        bestLength = key.second.size();
        best = &handler;
        param = path.substr(key.second.size());
      }
    }
    return best;
  }

  /**
   * Returns an error response when the request may not be served.
   *
   * A browser can be made to issue cross-origin requests to a loopback port by any page the user
   * visits, so an un-allow-listed Origin is refused outright rather than merely left without CORS
   * headers -- a bearer token alone would not stop a page that has somehow learned it.
   */
  [[nodiscard]] std::optional<Response> reject(const bhttp::request<bhttp::string_body>& req) const {
    const auto origin = req[bhttp::field::origin];
    if(!origin.empty() && !allowedOrigins.contains(std::string(origin))) {
      return Response::error(403, "Origin not allowed");
    }

    const auto auth = req[bhttp::field::authorization];
    const std::string expected = "Bearer " + authToken;
    if(auth.empty() || std::string(auth) != expected) {
      return Response::error(401, "Missing or invalid bearer token");
    }
    return std::nullopt;
  }

  void applyCors(const bhttp::request<bhttp::string_body>& req, auto& res) const {
    const auto origin = req[bhttp::field::origin];
    if(!origin.empty() && allowedOrigins.contains(std::string(origin))) {
      res.set(bhttp::field::access_control_allow_origin, origin);
      res.set(bhttp::field::access_control_allow_headers, "Authorization, Content-Type");
      res.set(bhttp::field::access_control_allow_methods, "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    }
  }

  void serveEvents(tcp::socket& socket, const bhttp::request<bhttp::string_body>& req) {
    auto connection = std::make_shared<EventConnection>();
    {
      const std::lock_guard<std::mutex> lock(connectionsMutex);
      connections.insert(connection);
    }

    std::string head =
        "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nCache-Control: no-cache\r\n"
        "Connection: keep-alive\r\nX-Accel-Buffering: no\r\n";
    if(const auto origin = req[bhttp::field::origin]; !origin.empty() && allowedOrigins.contains(std::string(origin))) {
      head += "Access-Control-Allow-Origin: " + std::string(origin) + "\r\n";
    }
    head += "\r\n";

    boost::system::error_code errorCode;
    asio::write(socket, asio::buffer(head), errorCode);

    while(!errorCode && !stopping) {
      std::deque<std::string> batch;
      {
        std::unique_lock<std::mutex> lock(connection->mutex);
        // A 15s wakeup doubles as the keep-alive comment below; without it a silent stream can sit
        // behind an idle-connection timeout with neither end noticing.
        connection->signal.wait_for(
            lock, std::chrono::seconds(15), [&] { return !connection->pending.empty() || connection->closed; });
        if(connection->closed) {
          break;
        }
        batch.swap(connection->pending);
      }

      if(batch.empty()) {
        asio::write(socket, asio::buffer(std::string(":keep-alive\n\n")), errorCode);
        continue;
      }

      for(const std::string& frame : batch) {
        asio::write(socket, asio::buffer(frame), errorCode);
        if(errorCode) {
          break;
        }
      }
    }

    {
      const std::lock_guard<std::mutex> lock(connectionsMutex);
      connections.erase(connection);
    }
  }

  void serve(tcp::socket socket) {
    beast::flat_buffer buffer;
    boost::system::error_code errorCode;

    {
      const std::lock_guard<std::mutex> lock(connectionsMutex);
      activeSockets.insert(&socket);
    }

    while(!stopping) {
      bhttp::request<bhttp::string_body> req;
      bhttp::read(socket, buffer, req, errorCode);
      if(errorCode) {
        break;
      }
      // Checked again after the read, not only at the top of the loop: a keep-alive connection sits
      // blocked in read across a stop(), and without this it would still answer the request that
      // unblocked it -- so a stopped engine would serve exactly one more call per open connection.
      if(stopping) {
        break;
      }

      Request request;
      request.method = std::string(bhttp::to_string(req.method()));
      request.body = req.body();
      parseTarget(std::string_view(req.target().data(), req.target().size()), request);

      // Preflight carries no Authorization header by design, so it is answered before the auth check.
      if(req.method() == bhttp::verb::options) {
        bhttp::response<bhttp::string_body> res{bhttp::status::no_content, req.version()};
        applyCors(req, res);
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        bhttp::write(socket, res, errorCode);
        if(errorCode) {
          break;
        }
        continue;
      }

      if(const std::optional<Response> refusal = reject(req); refusal.has_value()) {
        bhttp::response<bhttp::string_body> res{static_cast<bhttp::status>(refusal->status), req.version()};
        res.set(bhttp::field::content_type, refusal->contentType);
        res.body() = refusal->body;
        res.keep_alive(false);
        res.prepare_payload();
        bhttp::write(socket, res, errorCode);
        break;
      }

      if(!eventPath.empty() && request.path == eventPath) {
        serveEvents(socket, req);
        break;
      }

      Response response;
      if(const Handler* handler = findRoute(request.method, request.path, request.param); handler != nullptr) {
        try {
          response = (*handler)(request);
        } catch(const std::exception& exception) {
          LOG_ERROR(std::string("Handler for ") + request.path + " threw: " + exception.what());
          response = Response::error(500, "Internal error");
        }
      } else {
        response = Response::error(404, "No such endpoint");
      }

      bhttp::response<bhttp::string_body> res{static_cast<bhttp::status>(response.status), req.version()};
      res.set(bhttp::field::content_type, response.contentType);
      applyCors(req, res);
      res.body() = std::move(response.body);
      res.keep_alive(req.keep_alive());
      res.prepare_payload();
      bhttp::write(socket, res, errorCode);
      if(errorCode || !req.keep_alive()) {
        break;
      }
    }

    {
      const std::lock_guard<std::mutex> lock(connectionsMutex);
      activeSockets.erase(&socket);
    }

    boost::system::error_code ignored;
    socket.shutdown(tcp::socket::shutdown_both, ignored);
  }
};

Server::Server(std::string authToken) : mImpl(std::make_shared<Impl>(std::move(authToken))) {}

Server::Server(Server&&) noexcept = default;
Server& Server::operator=(Server&&) noexcept = default;

Server::~Server() {
  stop();
}

void Server::route(std::string method, std::string path, Handler handler) {
  const bool isPrefix = !path.empty() && path.back() == '/';
  auto& table = isPrefix ? mImpl->prefixRoutes : mImpl->exactRoutes;
  table[{std::move(method), std::move(path)}] = std::move(handler);
}

Response Server::dispatch(const std::string& method, const std::string& target, const std::string& body) const {
  Request request;
  request.method = method;
  request.body = body;
  parseTarget(target, request);

  const Handler* handler = mImpl->findRoute(request.method, request.path, request.param);
  if(handler == nullptr) {
    return Response::error(404, "Not found");
  }
  return (*handler)(request);
}

void Server::eventStream(std::string path) {
  mImpl->eventPath = std::move(path);
}

void Server::allowOrigin(std::string origin) {
  mImpl->allowedOrigins.insert(std::move(origin));
}

const std::string& Server::authToken() const {
  return mImpl->authToken;
}

void Server::broadcast(std::string_view name, std::string_view data) {
  std::string frame = "event: ";
  frame += name;
  frame += "\ndata: ";
  frame += data;
  frame += "\n\n";

  const std::lock_guard<std::mutex> lock(mImpl->connectionsMutex);
  for(const auto& connection : mImpl->connections) {
    connection->push(frame);
  }
}

uint16_t Server::start(uint16_t port) {
  boost::system::error_code errorCode;
  // 127.0.0.1 rather than "localhost": on Windows the latter may resolve to ::1 first, leaving a
  // client that dialed the IPv4 loopback unable to connect.
  const tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), port);

  mImpl->acceptor = std::make_unique<tcp::acceptor>(mImpl->io);
  mImpl->acceptor->open(endpoint.protocol(), errorCode);
  if(errorCode) {
    LOG_ERROR("Failed to open HTTP acceptor: " + errorCode.message());
    return 0;
  }
  mImpl->acceptor->set_option(asio::socket_base::reuse_address(true), errorCode);
  mImpl->acceptor->bind(endpoint, errorCode);
  if(errorCode) {
    LOG_ERROR("Failed to bind HTTP acceptor: " + errorCode.message());
    return 0;
  }
  mImpl->acceptor->listen(asio::socket_base::max_listen_connections, errorCode);
  if(errorCode) {
    LOG_ERROR("Failed to listen on HTTP acceptor: " + errorCode.message());
    return 0;
  }

  const uint16_t assigned = mImpl->acceptor->local_endpoint().port();
  mImpl->boundPort = assigned;

  mImpl->acceptThread = std::thread([impl = mImpl]() {
    while(!impl->stopping) {
      boost::system::error_code acceptError;
      tcp::socket socket(impl->io);
      impl->acceptor->accept(socket, acceptError);
      if(impl->stopping) {
        break;
      }
      if(acceptError) {
        LOG_WARNING("HTTP accept failed: " + acceptError.message());
        continue;
      }
      std::thread([impl, sock = std::move(socket)]() mutable { impl->serve(std::move(sock)); }).detach();
    }
  });

  return assigned;
}

void Server::stop() {
  // Null after a move-from; the moved-to Server owns the running threads.
  if(!mImpl || mImpl->stopping.exchange(true)) {
    return;
  }

  {
    const std::lock_guard<std::mutex> lock(mImpl->connectionsMutex);
    for(const auto& connection : mImpl->connections) {
      connection->close();
    }
    for(tcp::socket* socket : mImpl->activeSockets) {
      boost::system::error_code ignored;
      socket->shutdown(tcp::socket::shutdown_both, ignored);
    }
  }

  // Closing the acceptor does not interrupt a thread already blocked in a synchronous accept() on
  // Linux, so the blocked call is woken with a throwaway connection instead. Without this the join
  // below never returns.
  if(mImpl->acceptThread.joinable()) {
    if(mImpl->boundPort != 0) {
      boost::system::error_code ignored;
      asio::io_context wakeupIo;
      tcp::socket wakeup(wakeupIo);
      wakeup.connect(tcp::endpoint(asio::ip::make_address("127.0.0.1"), mImpl->boundPort), ignored);
      wakeup.close(ignored);
    }
    mImpl->acceptThread.join();
  }

  if(mImpl->acceptor) {
    boost::system::error_code ignored;
    mImpl->acceptor->close(ignored);
  }
}

}    // namespace http
