#include "ProtoJson.h"

#include <google/protobuf/util/json_util.h>

#include "logging.h"

namespace proto::json {

namespace {

google::protobuf::util::JsonPrintOptions printOptions() {
  google::protobuf::util::JsonPrintOptions options;
  // A client that reads a missing field as "absent" rather than "default" would misread an empty
  // graph or a zero error count, so defaults are spelled out.
  options.always_print_primitive_fields = true;
  options.preserve_proto_field_names = true;
  return options;
}

google::protobuf::util::JsonParseOptions parseOptions() {
  google::protobuf::util::JsonParseOptions options;
  // A field the engine does not know is a client/engine version skew, not a reason to fail the call.
  options.ignore_unknown_fields = true;
  return options;
}

}    // namespace

std::string toJson(const google::protobuf::Message& message) {
  std::string out;
  // Logged rather than swallowed: returning "{}" quietly would turn a serialization bug into an
  // empty graph or an empty result list, which looks exactly like "nothing found".
  if(const auto status = google::protobuf::util::MessageToJsonString(message, &out, printOptions()); !status.ok()) {
    LOG_ERROR("Failed to encode " + message.GetTypeName() + " as JSON: " + std::string(status.message()));
    return "{}";
  }
  return out;
}

bool fromJson(const std::string& text, google::protobuf::Message& message) {
  if(text.empty()) {
    // An empty body is a valid request for the endpoints whose input is entirely in the path/query.
    return true;
  }
  return google::protobuf::util::JsonStringToMessage(text, &message, parseOptions()).ok();
}

}    // namespace proto::json
