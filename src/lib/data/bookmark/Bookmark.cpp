#include "Bookmark.h"

Bookmark::Bookmark(
    Id id, const std::wstring& name, const std::wstring& comment, const TimeStamp& timeStamp, const BookmarkCategory& category)
    : mId(id), mName(name), mComment(comment), mTimeStamp(timeStamp), mCategory(category) {}

Bookmark::~Bookmark() = default;

Id Bookmark::getId() const {
  return mId;
}

void Bookmark::setId(const Id id) {
  mId = id;
}

std::wstring Bookmark::getName() const {
  return mName;
}

void Bookmark::setName(const std::wstring& name) {
  mName = name;
}

std::wstring Bookmark::getComment() const {
  return mComment;
}

void Bookmark::setComment(const std::wstring& comment) {
  mComment = comment;
}

TimeStamp Bookmark::getTimeStamp() const {
  return mTimeStamp;
}

void Bookmark::setTimeStamp(const TimeStamp& timeStamp) {
  mTimeStamp = timeStamp;
}

BookmarkCategory Bookmark::getCategory() const {
  return mCategory;
}

void Bookmark::setCategory(const BookmarkCategory& category) {
  mCategory = category;
}

bool Bookmark::isValid() const {
  return mIsValid;
}

void Bookmark::setIsValid(bool isValid) {
  mIsValid = isValid;
}
