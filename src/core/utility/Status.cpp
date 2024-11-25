#include "Status.h"

Status::Status(std::wstring msg, bool isErr)
    : message(std::move(msg)), type(isErr ? StatusType::STATUS_ERROR : StatusType::STATUS_INFO) {}