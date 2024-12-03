#pragma once
#include <vector>

template <typename T>
struct Tree {
  Tree() = default;

  Tree(T data_) : data(data_) {}

  T data;
  std::vector<Tree<T>> children;
};