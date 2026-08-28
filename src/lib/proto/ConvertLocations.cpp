#include "ConvertLocations.h"

#include "data/location/SourceLocation.h"
#include "data/location/SourceLocationCollection.h"
#include "data/location/SourceLocationFile.h"
#include "FilePath.h"
#include "utilityString.h"

namespace proto::convert {

sourcetrail::ProtoSourceLocationFile toProto(const SourceLocationFile& file) {
  sourcetrail::ProtoSourceLocationFile msg;

  msg.set_file_path(utility::encodeToUtf8(file.getFilePath().wstr()));
  msg.set_language(utility::encodeToUtf8(file.getLanguage()));
  msg.set_is_whole(file.isWhole());
  msg.set_is_complete(file.isComplete());
  msg.set_is_indexed(file.isIndexed());

  // Only start locations are walked: each carries its end via getOtherLocation(), and emitting both
  // halves would duplicate every location on the far side.
  file.forEachStartSourceLocation([&msg](SourceLocation* location) {
    if(location == nullptr) {
      return;
    }
    const SourceLocation* end = location->getEndLocation();
    if(end == nullptr) {
      return;
    }

    auto* locationMsg = msg.add_locations();
    locationMsg->set_location_id(location->getLocationId());
    for(const Id tokenId : location->getTokenIds()) {
      locationMsg->add_token_ids(tokenId);
    }
    locationMsg->set_type(static_cast<int>(location->getType()));
    locationMsg->set_start_line(location->getLineNumber());
    locationMsg->set_start_col(location->getColumnNumber());
    locationMsg->set_end_line(end->getLineNumber());
    locationMsg->set_end_col(end->getColumnNumber());
  });

  return msg;
}

std::shared_ptr<SourceLocationFile> fromProto(const sourcetrail::ProtoSourceLocationFile& msg) {
  auto file = std::make_shared<SourceLocationFile>(FilePath(utility::decodeFromUtf8(msg.file_path())),
                                                   utility::decodeFromUtf8(msg.language()),
                                                   msg.is_whole(),
                                                   msg.is_complete(),
                                                   msg.is_indexed());

  for(const auto& locationMsg : msg.locations()) {
    std::vector<Id> tokenIds;
    tokenIds.reserve(static_cast<size_t>(locationMsg.token_ids_size()));
    for(const auto tokenId : locationMsg.token_ids()) {
      tokenIds.push_back(tokenId);
    }

    file->addSourceLocation(static_cast<LocationType>(locationMsg.type()),
                            locationMsg.location_id(),
                            std::move(tokenIds),
                            locationMsg.start_line(),
                            locationMsg.start_col(),
                            locationMsg.end_line(),
                            locationMsg.end_col());
  }

  return file;
}

sourcetrail::SourceLocationCollectionResponse toProto(const SourceLocationCollection& collection) {
  sourcetrail::SourceLocationCollectionResponse msg;

  collection.forEachSourceLocationFile([&msg](const std::shared_ptr<SourceLocationFile>& file) {
    if(file != nullptr) {
      *msg.add_files() = toProto(*file);
    }
  });

  return msg;
}

std::shared_ptr<SourceLocationCollection> fromProto(const sourcetrail::SourceLocationCollectionResponse& msg) {
  auto collection = std::make_shared<SourceLocationCollection>();

  for(const auto& fileMsg : msg.files()) {
    collection->addSourceLocationFile(fromProto(fileMsg));
  }

  return collection;
}

}    // namespace proto::convert
