#pragma once
#include <algorithm>

#include <QObject>
#include <QString>

namespace utilities {
template <class Type>
Type* getChildByText(const QObject& object, const QString& name) {
  auto types = object.findChildren<Type*>();
  if(types.empty()) {
    return nullptr;
  }
  if(types.size() == 1) {
    auto* type = types.front();
    return (type->text() == name) ? type : nullptr;
  }
  auto itr = std::find_if(std::cbegin(types), std::cend(types), [&name](auto* type) { return type->text() == name; });
  return (itr == std::cend(types)) ? nullptr : *itr;
}

template <class Type>
Type* getChildByObjectName(const QObject& object, const QString& name) {
  auto types = object.findChildren<Type*>();
  if(types.empty()) {
    return nullptr;
  }
  if(types.size() == 1) {
    auto* type = types.front();
    return (type->objectName() == name) ? type : nullptr;
  }
  for(auto* type : types) {
    if(type->objectName() == name) {
      return type;
    }
  }
  return nullptr;
}
}    // namespace utilities
