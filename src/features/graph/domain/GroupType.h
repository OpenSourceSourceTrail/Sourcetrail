#pragma once

#include <string>

enum class GroupType { NONE, DEFAULT, FRAMELESS, FILE_TYPE, NAMESPACE, INHERITANCE };

std::wstring groupTypeToString(GroupType type);
GroupType stringToGroupType(const std::wstring& value);

enum class GroupLayout { LIST, SKEWED, BUCKET, SQUARE };