#include "BookmarkCategory.h"

BookmarkCategory::BookmarkCategory() : m_id(static_cast<Id>(-1)) {}

BookmarkCategory::BookmarkCategory(const Id id, const std::wstring& name) : m_id(id), m_name(name) {}

BookmarkCategory::~BookmarkCategory() {}

Id BookmarkCategory::getId() const {
  return m_id;
}

void BookmarkCategory::setId(const Id id) {
  m_id = id;
}

std::wstring BookmarkCategory::getName() const {
  return m_name;
}

void BookmarkCategory::setName(const std::wstring& name) {
  m_name = name;
}
