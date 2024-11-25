#pragma once
#include <string>
#include <vector>

#include "BookmarkCategory.h"
#include "TimeStamp.h"
#include "GlobalId.hpp"

class Bookmark {
public:
  enum class Filter : uint8_t { Unknown = 0, All, Nodes, Edges };

  enum class Order : uint8_t { None = 0, DateAscending, DateDescending, NameAscending, NameDescending };

  Bookmark(Id id, const std::wstring& name, const std::wstring& comment, const TimeStamp& timeStamp, const BookmarkCategory& category);
  virtual ~Bookmark();

  Id getId() const;
  void setId(const Id id);

  std::wstring getName() const;
  void setName(const std::wstring& name);

  std::wstring getComment() const;
  void setComment(const std::wstring& comment);

  TimeStamp getTimeStamp() const;
  void setTimeStamp(const TimeStamp& timeStamp);

  BookmarkCategory getCategory() const;
  void setCategory(const BookmarkCategory& category);

  bool isValid() const;
  void setIsValid(bool isValid = true);

private:
  Id mId;
  std::wstring mName;
  std::wstring mComment;
  TimeStamp mTimeStamp;
  BookmarkCategory mCategory;
  bool mIsValid = false;
};