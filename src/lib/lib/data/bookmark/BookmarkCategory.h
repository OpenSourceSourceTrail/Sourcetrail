#ifndef BOOKMARK_CATEGORY_H
#define BOOKMARK_CATEGORY_H

#include <string>
#include <string_view>

#include "GlobalId.hpp"

/**
 * Category a bookmark falls into when its creator names none.
 *
 * A bookmark row's category_id is a foreign key, so leaving the category empty makes the insert
 * fail outright -- every path that creates a bookmark has to substitute this.
 */
inline constexpr std::wstring_view BookmarkDefaultCategoryName = L"default";

class BookmarkCategory {
public:
  BookmarkCategory();
  BookmarkCategory(const Id id, const std::wstring& name);
  ~BookmarkCategory();

  Id getId() const;
  void setId(const Id id);

  std::wstring getName() const;
  void setName(const std::wstring& name);

private:
  Id m_id;
  std::wstring m_name;
};

#endif    // BOOKMARK_CATEGORY_H
