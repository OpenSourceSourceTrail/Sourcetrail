#pragma once
#include <functional>
#include <mutex>

#include <unordered_map>

template <typename KeyType, typename ValType, typename Hasher = std::hash<KeyType>>
class UnorderedCache final {
public:
  explicit UnorderedCache(std::function<ValType(const KeyType&)> calculator);

  ValType getValue(const KeyType& key);

private:
  std::function<ValType(const KeyType&)> m_calculator;
  std::unordered_map<KeyType, ValType, Hasher> m_map;
  std::mutex m_mutex;

  size_t m_hitCount{};
  size_t m_missCount{};
};

template <typename KeyType, typename ValType, typename Hasher>
UnorderedCache<KeyType, ValType, Hasher>::UnorderedCache(std::function<ValType(const KeyType&)> calculator)
    : m_calculator(std::move(calculator)) {}

template <typename KeyType, typename ValType, typename Hasher>
ValType UnorderedCache<KeyType, ValType, Hasher>::getValue(const KeyType& key) {
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto iterator = m_map.find(key);
  if(iterator != m_map.end()) {
    ++m_hitCount;
    return iterator->second;
  }
  ++m_missCount;
  ValType val = m_calculator(key);
  m_map.emplace(key, val);
  return val;
}