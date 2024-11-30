#pragma once
#include <functional>
#include <map>

template <typename KeyType, typename ValType>
class OrderedCache final {
public:
  OrderedCache(std::function<ValType(const KeyType&)> calculator);
  ValType getValue(const KeyType& key);

private:
  std::function<ValType(const KeyType&)> mCalculator;
  std::map<KeyType, ValType> mMap;

  size_t mHitCount = 0;
  size_t mMissCount = 0;
};

template <typename KeyType, typename ValType>
OrderedCache<KeyType, ValType>::OrderedCache(std::function<ValType(const KeyType&)> calculator)
    : mCalculator(std::move(calculator)) {}

template <typename KeyType, typename ValType>
ValType OrderedCache<KeyType, ValType>::getValue(const KeyType& key) {
  if(auto found = mMap.find(key); found != mMap.end()) {
    ++mHitCount;
    return found->second;
  }
  ++mMissCount;
  ValType val = mCalculator(key);
  mMap.emplace(key, val);
  return val;
}
