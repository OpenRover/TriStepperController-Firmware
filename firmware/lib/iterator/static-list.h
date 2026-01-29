// =============================================================================
// License: MIT
// Author: Yuxuan Zhang (zhangyuxuan@ufl.edu)
// =============================================================================

#pragma once

#include <cstddef>
#include <stdexcept>

template <typename T, size_t N> class List {
private:
  T *items[N];
  size_t count = 0;

public:
  List() = default;

  List<T, N> &add(T *item) {
    if (count < N) {
      items[count++] = item;
    } else {
      throw std::runtime_error("List::add(T *) will cause overflow");
    }
    return *this;
  }

  List<T, N> &add(T &item) {
    if (count < N) {
      items[count++] = &item;
    } else {
      throw std::runtime_error("List::add(T &) will cause overflow");
    }
    return *this;
  }

  size_t size() const { return count; }

  T &operator[](size_t index) const {
    if (index < count) {
      return *items[index];
    } else {
      throw std::runtime_error("List::Operator[]: Index out of range");
    }
  }

  List<T, N> &clear() {
    count = 0;
    return *this;
  }

  // Support range-based for loops
  class Ptr {
  private:
    T **ptr;

  public:
    Ptr(T **p) : ptr(p) {}
    T &operator*() const { return **ptr; }
    Ptr &operator++() {
      ++ptr;
      return *this;
    }
    bool operator!=(const Ptr &other) const { return ptr != other.ptr; }
  };
  Ptr begin() { return Ptr(items); }
  Ptr end() { return Ptr(items + count); }
};
