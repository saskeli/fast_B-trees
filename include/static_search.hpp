#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <utility>
#include <vector>

#include "internal.hpp"

#ifndef CACHE_LINE
#define CACHE_LINE 64
#endif

namespace bt {

template <class T, uint16_t block_size = 8>
class static_set {
 private:
  static_assert(__builtin_popcountll(block_size) == 1);
  static_assert(block_size <= 8192);
  static_assert(block_size >= 2);

  class node {
   public:
    std::array<T, block_size> elements;

    node() {}

    size_t find(T q) const { return internal::search(elements, q); }
  };

  T max_value_;
  node* nodes_;
  size_t size_;
  size_t node_count_;
  size_t internal_nodes_;
  size_t max_count_;
  uint16_t levels_;

 public:
  static_set(T const* data, size_t n, const T& max_value)
      : max_value_(max_value),
        nodes_(nullptr),
        size_(n),
        node_count_(0),
        internal_nodes_(0),
        max_count_(0),
        levels_(0) {
    for (size_t i = n - 1; i < n; --i) {
      if (data[i] >= max_value_) {
        ++max_count_;
      } else {
        break;
      }
    }
    size_t nn = n;
    size_t leaves = (n + block_size - 1) / block_size;
    while (nn > block_size) {
      nn = (nn + block_size - 1) / block_size;
      levels_++;
    }
    if (leaves > 1) {
      size_t n_lev = 1;
      internal_nodes_ = 1;
      for (size_t i = 1; i < levels_; i++) {
        n_lev *= block_size;
        internal_nodes_ += n_lev;
      }
      node_count_ = internal_nodes_ + leaves;
    } else {
      node_count_ = leaves;
    }
    nodes_ = new node[node_count_];
    for (size_t i = 0; i < node_count_; ++i) {
      std::fill_n(nodes_[i].elements.data(), block_size, max_value_);
    }
    T* leaf_elems = reinterpret_cast<T*>(nodes_ + internal_nodes_);
    std::memcpy(leaf_elems, data, n * sizeof(T));
    std::sort(leaf_elems, leaf_elems + n);
    for (uint64_t i = internal_nodes_ - 1; i < internal_nodes_; i--) {
      for (uint64_t k = 0; k < block_size; k++) {
        uint64_t c_idx = i * block_size + k + 1;
        if (c_idx < node_count_) {
          nodes_[i].elements[k] = nodes_[i * block_size + k + 1].elements[0];
        }
      }
    }
  }

  static_set(T const* data, size_t n)
      : static_set(data, n, internal::max_val<T>()) {}

  static_set(const T& max_value)
      : max_value_(max_value),
        nodes_(nullptr),
        size_(0),
        node_count_(0),
        internal_nodes_(0),
        max_count_(0),
        levels_(0) {}

  static_set() : static_set(internal::max_val<T>()) {}

  static_set(std::vector<T> const& vec, const T& max_value)
      : static_set(vec.data(), vec.size(), max_value) {}

  static_set(std::vector<T> const& vec)
      : static_set(vec.data(), vec.size()) {}

  ~static_set() {
    if (nodes_ != nullptr) {
      delete[] (nodes_);
    }
  }

  static_set(const static_set& other) {
    max_value_ = other.max_value_;
    node_count_ = other.node_count_;
    size_ = other.size_;
    levels_ = other.levels_;
    internal_nodes_ = other.internal_nodes_;
    max_count_ = other.max_count_;
    nodes_ = new node[node_count_]();
    std::copy_n(other.nodes_, node_count_, nodes_);
  }

  static_set(static_set&& other) {
    max_value_ = other.max_value_;
    node_count_ = std::exchange(other.node_count_, 0);
    size_ = std::exchange(other.size_, 0);
    levels_ = std::exchange(other.levels_, 0);
    internal_nodes_ = std::exchange(other.internal_nodes_, 0);
    max_count_ = std::exchange(other.max_count_, 0);
    nodes_ = std::exchange(other.nodes_, nullptr);
  }

  static_set& operator=(const static_set& other) {
    max_value_ = other.max_value_;
    node_count_ = other.node_count_;
    size_ = other.size_;
    levels_ = other.levels_;
    internal_nodes_ = other.internal_nodes_;
    max_count_ = other.max_count_;
    nodes_ = new node[node_count_]();
    std::copy_n(other.nodes_, node_count_, nodes_);
    return *this;
  }

  static_set& operator=(static_set&& other) {
    max_value_ = other.max_value_;
    node_count_ = std::exchange(other.node_count_, 0);
    size_ = std::exchange(other.size_, 0);
    levels_ = std::exchange(other.levels_, 0);
    internal_nodes_ = std::exchange(other.internal_nodes_, 0);
    max_count_ = std::exchange(other.max_count_, 0);
    nodes_ = std::exchange(other.nodes_, nullptr);
    return *this;
  }

  const T* begin() const {
    if (size_ == 0) [[unlikely]] {
      return nullptr;
    }
    return reinterpret_cast<T*>(nodes_) + internal_nodes_ * block_size;
  }

  const T* end() const {
    if (size_ == 0) [[unlikely]] {
      return nullptr;
    }
    return nodes_[internal_nodes_].elements.data() + size_;
  }

  size_t size() const { return size_; }

  bool contains(const T& q) const {
    if (size_ == 0) [[unlikely]] {
      return false;
    }
    if (q == max_value_) [[unlikely]] {
      return max_count_ > 0;
    }
    size_t n_idx = 0;
    for (size_t i = 0; i < levels_; i++) {
      if constexpr (block_size <= 16) {
        const constexpr size_t tot_bytes = sizeof(node) * block_size;
        uint8_t const* byte_data =
            reinterpret_cast<uint8_t const*>(nodes_ + (n_idx * block_size + 1));

        for (size_t c_idx = 0; c_idx < tot_bytes; c_idx += CACHE_LINE) {
          __builtin_prefetch(byte_data + c_idx);
        }
      }
      uint16_t res = nodes_[n_idx].find(q);
      n_idx = n_idx * block_size + 1 + nodes_[n_idx].find(q);
    }
    return nodes_[n_idx].elements.at(nodes_[n_idx].find(q)) == q;
  }

  const T* predecessor(const T& q) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    if (q == max_value_) [[unlikely]] {
      auto e = end();
      return --e;
    }
    size_t n_idx = 0;
    for (size_t i = 0; i < levels_; ++i) {
      if constexpr (block_size <= 16) {
        const constexpr size_t tot_bytes = sizeof(node) * block_size;
        uint8_t const* byte_data =
            reinterpret_cast<uint8_t const*>(nodes_ + (n_idx * block_size + 1));
        for (size_t c_idx = 0; c_idx < tot_bytes; c_idx += CACHE_LINE) {
          __builtin_prefetch(byte_data + c_idx);
        }
      }
      uint16_t res = nodes_[n_idx].find(q);
      n_idx = n_idx * block_size + 1 + res;
    }
    size_t internal_idx = nodes_[n_idx].find(q);
    if (nodes_[n_idx].elements[internal_idx] > q) {
      return end();
    }
    return nodes_[n_idx].elements.data() + internal_idx;
  }

  const T* lower_bound(const T& q) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    const T* ptr = predecessor(q);
    if (ptr == end()) [[unlikely]] {
      return begin();
    }
    const T* b_ptr = begin();
    while (ptr >= b_ptr && *ptr == q) {
      --ptr;
    }
    return ++ptr;
  }

  const T* upper_bound(const T& q) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    const T* ptr = predecessor(q);
    if (ptr == end()) [[unlikely]] {
      return begin();
    }
    return ++ptr;
  }

  size_t count(const T& q) const { return upper_bound(q) - lower_bound(q); }

  size_t bytes() const {
    return sizeof(static_set) + node_count_ * sizeof(node);
  }
};

template <class K, class V, uint16_t block_size = 8>
class static_map {
  class map_iter {
    const K* current_key;
    const V* current_val;

   public:
    map_iter(const K* key, const V* val) : current_key(key), current_val(val) {}

    std::pair<K, V> operator*() const { return {*current_key, *current_val}; }

    map_iter& operator++() {
      ++current_key;
      ++current_val;
      return *this;
    }

    map_iter operator++(int) {
      auto ret_val = *this;
      ++current_key;
      ++current_val;
      return ret_val;
    }

    bool operator==(const map_iter& rhs) const {
      return current_key == rhs.current_key && current_val == rhs.current_val;
    }

    bool operator!=(const map_iter& rhs) const { return !(*this == rhs); }

    template <class integer_t>
    map_iter operator+(integer_t a) const {
      return {current_key + a, current_val + a};
    }

    std::ptrdiff_t operator-(const map_iter& rhs) const {
      return current_key - rhs.current_key;
    }

    template <class integer_t>
    map_iter operator-(integer_t a) const {
      return {current_key - a, current_val - a};
    }
  };

  static_set<K, block_size> set_;
  V* values_;

  static std::vector<K> merge_sort_and_flip(const K* k_data, const V* v_data,
                                            size_t n, V* val_ref) {
    std::vector<std::pair<K, V>> i_vec;
    for (size_t i = 0; i < n; ++i) {
      i_vec.push_back({k_data[i], v_data[i]});
    }
    return sort_and_flip(i_vec, val_ref);
  }

  static std::vector<K> sort_and_flip(std::vector<std::pair<K, V>>& i_vec,
                                      V* val_ref) {
    std::sort(i_vec.begin(), i_vec.end());
    std::vector<K> k_vec;
    for (size_t i = 0; i < i_vec.size(); ++i) {
      k_vec.push_back(i_vec[i].first);
      val_ref[i] = i_vec[i].second;
    }
    return k_vec;
  }

 public:
  static_map(const K* k_data, const V* v_data, size_t n, const K& max_value) {
    values_ = new V[n];
    for (size_t i = 1; i < n; ++i) {
      if (k_data[i - 1] > k_data[i]) {
        auto sorted_k = merge_sort_and_flip(k_data, v_data, n, values_);
        set_ = {sorted_k, max_value};
        return;
      }
    }
    set_ = {k_data, n, max_value};
    std::copy_n(v_data, n, values_);
  }

  static_map(const K* k_data, const V* v_data, size_t n)
      : static_map(k_data, v_data, n, internal::max_val<K>()) {}

  static_map(const std::pair<K, V>* data, size_t n, const K& max_value) {
    values_ = new V[n];
    for (size_t i = 1; i < n; ++i) {
      if (data[i - 1].first > data[i].first) {
        std::vector<std::pair<K, V>> i_vec(data, data + n);
        auto sorted_k = sort_and_flip(i_vec, values_);
        set_ = {sorted_k};
        return;
      }
    }
    std::vector<K> k_vec;
    for (size_t i = 0; i < n; ++i) {
      k_vec.push_back(data[i].first);
      values_[i] = data[i].second;
    }
    set_ = {k_vec};
  }

  static_map(const std::pair<K, V>* data, size_t n)
      : static_map(data, n, internal::max_val<K>()) {}

  static_map(std::vector<std::pair<K, V>> const& vec)
      : static_map(vec.data(), vec.size()) {}

  static_map(std::vector<K> const& k_vec, std::vector<V> const& v_vec)
      : static_map(k_vec.data(), v_vec.data(), k_vec.size()) {}

  map_iter begin() const { return {set_.begin(), values_}; }

  map_iter end() const { return {set_.end(), values_ + size()}; }

  size_t size() const { return set_.size(); }

  bool contains_key(const K& q) const { return set_.contains(q); }

  const map_iter predecessor(const K& q) const {
    if (size() == 0) [[unlikely]] {
      return end();
    }
    const K* ptr = set_.predecessor(q);
    return {ptr, values_ + (ptr - set_.begin())};
  }

  const map_iter lower_bound(const K& q) const {
    if (size() == 0) [[unlikely]] {
      return end();
    }
    const K* ptr = set_.lower_bound(q);
    return {ptr, values_ + (ptr - set_.begin())};
  }

  const map_iter upper_bound(const K& q) const {
    if (size() == 0) [[unlikely]] {
      return end();
    }
    const K* ptr = set_.upper_bound(q);
    return {ptr, values_ + (ptr - set_.begin())};
  }

  size_t count(const K& q) const { return set_.count(q); }

  size_t bytes() const {
    return sizeof(static_map) + size() * sizeof(V) + set_.bytes();
  }
};

}  // namespace bt