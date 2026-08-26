#pragma once

#include <string>

#include <google/protobuf/message.h>

namespace proto::json {

/**
 * Canonical protobuf JSON, the wire format of the client <-> engine boundary.
 *
 * Using protobuf's own mapping rather than a hand-written serializer is what lets engine.proto stay
 * the single schema for a boundary that no longer speaks gRPC: the Convert*.cpp helpers keep
 * producing the same messages, and only the encoding downstream of them changed.
 *
 * Fields holding their default value are emitted explicitly, so a JSON consumer sees a complete
 * object instead of having to know each field's default. Note that uint64 fields are rendered as
 * strings -- a JSON number cannot represent 64 bits -- so ids arrive quoted.
 */
std::string toJson(const google::protobuf::Message& message);

/** Returns false when `text` is not valid JSON for `message`; `message` is then left unspecified. */
bool fromJson(const std::string& text, google::protobuf::Message& message);

}    // namespace proto::json
