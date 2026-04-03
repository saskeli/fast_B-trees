#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "internal.hpp"

namespace bt {
namespace internal {
template <class T, class leaf_t, uint16_t block_size>
class dynamic_tree {
  class node {
    template <uint16_t b = block_size>
    static uint16_t elem_count(void* const* children) {
      if constexpr (b == 1) {
        return *children != nullptr;
      }
      uint16_t offset = (b / 2) * (children[b / 2 - 1] != nullptr);
      uint16_t res = elem_count<b / 2>(children + offset);
      return offset + res;
    }

   public:
    std::array<T, block_size> items;
    std::array<void*, block_size> child_pointers;

    node(const T& mv) : items(), child_pointers() {
      items.fill(mv);
      child_pointers.fill(nullptr);
    }

    uint16_t size() const { return elem_count(child_pointers.data()); }

    uint16_t find(const T& q, const T& mv) const {
      if (q == mv) [[unlikely]] {
        uint16_t s = size();
        return s > 0 ? s - 1 : s;
      }
      return internal::search(items, q);
    }

    bool is_full() const { return child_pointers[block_size - 1] != nullptr; }

    bool half_empty() const {
      return child_pointers[block_size / 2] == nullptr;
    }

    node* split(const T& mv) {
      node* new_node = new node(mv);
      for (uint16_t i = 0; i < block_size / 2; ++i) {
        uint16_t s_idx = i + block_size / 2;
        new_node->items[i] = std::exchange(items[s_idx], mv);
        new_node->child_pointers[i] =
            std::exchange(child_pointers[s_idx], nullptr);
      }
      return new_node;
    }

    template <bool child_is_leaf>
    void split_child(uint16_t idx, const T& mv) {
      if constexpr (child_is_leaf) {
        leaf_t* old_leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        leaf_t* new_leaf = old_leaf->split(mv);
        for (uint16_t i = block_size - 1; i > idx + 1; --i) {
          items[i] = items[i - 1];
          child_pointers[i] = child_pointers[i - 1];
        }
        items[idx + 1] = new_leaf->items[0];
        child_pointers[idx + 1] = new_leaf;
      } else {
        node* old_node = reinterpret_cast<node*>(child_pointers[idx]);
        node* new_node = old_node->split(mv);
        for (uint16_t i = block_size - 1; i > idx + 1; --i) {
          items[i] = items[i - 1];
          child_pointers[i] = child_pointers[i - 1];
        }
        items[idx + 1] = new_node->items[0];
        child_pointers[idx + 1] = new_node;
      }
    }

    template <bool child_is_leaf>
    void merge(uint16_t idx, const T& mv) {
      if constexpr (child_is_leaf) {
        leaf_t* m_leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        uint16_t m_size = m_leaf->size();
        leaf_t* l_leaf = nullptr;
        uint16_t l_size = 0;
        if (idx > 0) {
          l_leaf = reinterpret_cast<leaf_t*>(child_pointers[idx - 1]);
          l_size = l_leaf->size();
        }
        leaf_t* r_leaf = nullptr;
        uint16_t r_size = 0;
        if (idx < block_size - 1 && child_pointers[idx + 1] != nullptr) {
          r_leaf = reinterpret_cast<leaf_t*>(child_pointers[idx + 1]);
          r_size = r_leaf->size();
        }
        if (l_size > r_size) {
          if (l_size == block_size / 2) {
            l_leaf->merge(m_leaf);
            for (uint16_t i = idx + 1; i < block_size; ++i) {
              items[i - 1] = items[i];
              child_pointers[i - 1] = child_pointers[i];
            }
            items[block_size - 1] = mv;
            child_pointers[block_size - 1] = nullptr;
          } else {
            l_leaf->balance(m_leaf);
            items[idx] = m_leaf->items[0];
          }
        } else if (r_size > block_size / 2) {
          m_leaf->balance(r_leaf);
          items[idx + 1] = r_leaf->items[0];
        } else {
          m_leaf->merge(r_leaf);
          for (uint16_t i = idx + 2; i < block_size; ++i) {
            items[i - 1] = items[i];
            child_pointers[i - 1] = child_pointers[i];
          }
          items[block_size - 1] = mv;
          child_pointers[block_size - 1] = nullptr;
        }
      } else {
        node* m_node = reinterpret_cast<node*>(child_pointers[idx]);
        uint16_t m_size = m_node->size();
        node* l_node = nullptr;
        uint16_t l_size = 0;
        if (idx > 0) {
          l_node = reinterpret_cast<node*>(child_pointers[idx - 1]);
          l_size = l_node->size();
        }
        node* r_node = nullptr;
        uint16_t r_size = 0;
        if (idx + 1 < block_size && child_pointers[idx + 1] != nullptr) {
          r_node = reinterpret_cast<node*>(child_pointers[idx + 1]);
          r_size = r_node->size();
        }
        if (l_size > r_size) {
          if (l_size == block_size / 2) {
            l_node->merge(m_node, l_size, m_size);
            for (uint16_t i = idx + 1; i < block_size; ++i) {
              items[i - 1] = items[i];
              child_pointers[i - 1] = child_pointers[i];
            }
            items[block_size - 1] = mv;
            child_pointers[block_size - 1] = nullptr;
          } else {
            l_node->balance(m_node, l_size, m_size);
            items[idx] = m_node->items[0];
          }
        } else if (r_size > block_size / 2) {
          m_node->balance(r_node, m_size, r_size);
          items[idx + 1] = r_node->items[0];
        } else {
          m_node->merge(r_node, m_size, r_size);
          for (uint16_t i = idx + 2; i < block_size; ++i) {
            items[i - 1] = items[i];
            child_pointers[i - 1] = child_pointers[i];
          }
          items[block_size - 1] = mv;
          child_pointers[block_size - 1] = nullptr;
        }
      }
    }

    void merge(node* other, uint16_t size, uint16_t other_size) {
      for (uint16_t i = 0; i < other_size; ++i) {
        items[i + size] = other->items[i];
        child_pointers[i + size] = other->child_pointers[i];
        other->child_pointers[i] = nullptr;
      }
      delete (other);
    }

    void balance(node* other, uint16_t size, uint16_t other_size) {
      if (size < other_size) {
        const T mv = items[block_size - 1];
        uint16_t copy_count = (1 + other_size - size) / 2;
        for (uint16_t i = 0; i < copy_count; ++i) {
          items[i + size] = other->items[i];
          child_pointers[i + size] = other->child_pointers[i];
        }
        for (uint16_t i = 0; i < other_size - copy_count; ++i) {
          other->items[i] = other->items[i + copy_count];
          other->items[i + copy_count] = mv;
          other->child_pointers[i] = other->child_pointers[i + copy_count];
          other->child_pointers[i + copy_count] = nullptr;
        }
      } else {
        const T mv = other->items[block_size - 1];
        uint16_t copy_count = (1 + size - other_size) / 2;
        std::move_backward(other->items.data(),
                           other->items.data() + other_size,
                           other->items.data() + other_size + copy_count);
        std::move_backward(
            other->child_pointers.data(),
            other->child_pointers.data() + other_size,
            other->child_pointers.data() + other_size + copy_count);
        for (uint16_t i = 0; i < copy_count; ++i) {
          other->items[i] = items[size - copy_count + i];
          items[size - copy_count + i] = mv;
          other->child_pointers[i] = child_pointers[size - copy_count + i];
          child_pointers[size - copy_count + i] = nullptr;
        }
      }
    }

    bool remove(const T& v, uint16_t height, const T& mv) {
      auto idx = find(v, mv);
      if (height <= 1) {
        leaf_t* leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        if (leaf->half_empty()) {
          merge<true>(idx, mv);
          idx = find(v, mv);
          leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        }
        bool removed = leaf->remove(v, mv);
        items[idx] = leaf->items[0];
        return removed;
      } else {
        node* child = reinterpret_cast<node*>(child_pointers[idx]);
        if (child->half_empty()) {
          merge<false>(idx, mv);
          idx = find(v, mv);
          child = reinterpret_cast<node*>(child_pointers[idx]);
        }
        bool removed = child->remove(v, height - 1, mv);
        items[idx] = child->items[0];
        return removed;
      }
    }
  };

  class bt_iterator {
    std::vector<std::pair<const node*, uint16_t>> node_stack_;
    const leaf_t* current_leaf_;
    uint16_t current_idx_;
    uint16_t levels_;

   public:
    bt_iterator(const bt_iterator& other)
        : node_stack_(other.node_stack_),
          current_leaf_(other.current_leaf_),
          current_idx_(other.current_idx_),
          levels_(other.levels_) {}

    bt_iterator(const node* root, uint16_t levels)
        : node_stack_(),
          current_leaf_(nullptr),
          current_idx_(0),
          levels_(levels) {
      if (root == nullptr) {
        return;
      }
      node_stack_.push_back({root, 0});
      std::pair<const node*, uint16_t> p;
      while (node_stack_.size() < levels_) {
        p = node_stack_.back();
        const node* next_node =
            reinterpret_cast<const node*>(p.first->child_pointers[p.second]);
        node_stack_.push_back({next_node, 0});
      }
      p = node_stack_.back();
      current_leaf_ =
          reinterpret_cast<const leaf_t*>(p.first->child_pointers[p.second]);
      current_idx_ = 0;
    }

    bt_iterator(const leaf_t* leaf)
        : node_stack_(), current_leaf_(leaf), current_idx_(0), levels_(0) {
      return;
    }

    bt_iterator(std::vector<std::pair<const node*, uint16_t>> node_stack,
                const leaf_t* leaf, uint16_t leaf_index, uint16_t levels)
        : node_stack_(node_stack),
          current_leaf_(leaf),
          current_idx_(leaf_index),
          levels_(levels) {}

    bt_iterator& operator++() {
      ++current_idx_;
      if (current_idx_ < current_leaf_->size()) {
        return *this;
      }
      current_leaf_ = nullptr;
      current_idx_ = 0;
      std::pair<const node*, uint16_t> p;
      while (node_stack_.size() > 0) {
        p = node_stack_.back();
        node_stack_.pop_back();
        uint16_t idx = p.second + 1;
        if (idx < block_size && p.first->child_pointers[idx] != nullptr) {
          node_stack_.push_back({p.first, idx});
          break;
        }
      }
      if (node_stack_.empty()) {
        return *this;
      }
      while (node_stack_.size() < levels_) {
        p = node_stack_.back();
        const node* next_node =
            reinterpret_cast<const node*>(p.first->child_pointers[p.second]);
        node_stack_.push_back({next_node, 0});
      }
      p = node_stack_.back();
      current_leaf_ =
          reinterpret_cast<const leaf_t*>(p.first->child_pointers[p.second]);
      current_idx_ = 0;
      return *this;
    }

    bt_iterator& operator--() {
      --current_idx_;
      if (current_idx_ < current_leaf_->size()) {
        return *this;
      }
      current_leaf_ = nullptr;
      current_idx_ = 0;
      std::pair<const node*, uint16_t> p;
      while (node_stack_.size() > 0) {
        p = node_stack_.back();
        node_stack_.pop_back();
        uint16_t idx = p.second - 1;
        if (idx < block_size) {
          node_stack_.push_back({p.first, idx});
          break;
        }
      }
      if (node_stack_.empty()) {
        return *this;
      }
      while (node_stack_.size() < levels_) {
        p = node_stack_.back();
        const node* next_node =
            reinterpret_cast<const node*>(p.first->child_pointers[p.second]);
        node_stack_.push_back({next_node, next_node->size() - 1});
      }
      p = node_stack_.back();
      current_leaf_ =
          reinterpret_cast<const leaf_t*>(p.first->child_pointers[p.second]);
      current_idx_ = current_leaf_->size() - 1;
      return *this;
    }

    bt_iterator operator--(int) {
      bt_iterator ret(*this);
      --(*this);
      return ret;
    }

    bt_iterator operator++(int) {
      bt_iterator ret(*this);
      ++(*this);
      return ret;
    }

    size_t operator-(const bt_iterator& rhs) const {
      bt_iterator stepper(rhs);
      size_t ret = 0;
      while (stepper != *this) {
        ++stepper;
        ++ret;
      }
      return ret;
    }

    bt_iterator operator+(size_t rhs) const {
      bt_iterator ret(*this);
      for (size_t i = 0; i < rhs; ++i) {
        ++ret;
      }
      return ret;
    }

    bool operator==(const bt_iterator& rhs) const {
      if (current_leaf_ != rhs.current_leaf_) {
        return false;
      }
      return (current_idx_ == rhs.current_idx_);
    }

    bool operator!=(const bt_iterator& rhs) const { return !(*this == rhs); }

    typename leaf_t::iter_deref_t operator*() const {
      return current_leaf_->get(current_idx_);
    }

    const T& key() const { return current_leaf_->items[current_idx_]; }
  };

  void* root_;
  T max_value_;
  uint16_t levels_;

  template <bool root_is_leaf>
  void split_root() {
    if constexpr (root_is_leaf) {
      leaf_t* old_root = reinterpret_cast<leaf_t*>(root_);
      node* new_root = new node(max_value_);
      root_ = new_root;
      new_root->items[0] = old_root->items[0];
      new_root->child_pointers[0] = old_root;
      leaf_t* new_leaf = old_root->split(max_value_);
      new_root->items[1] = new_leaf->items[0];
      new_root->child_pointers[1] = new_leaf;
      ++levels_;
    } else {
      node* left_node = reinterpret_cast<node*>(root_);
      node* new_root = new node(max_value_);
      root_ = new_root;
      new_root->items[0] = left_node->items[0];
      new_root->child_pointers[0] = left_node;
      node* right_node = left_node->split(max_value_);
      new_root->items[1] = right_node->items[0];
      new_root->child_pointers[1] = right_node;
      ++levels_;
    }
  }

  template <bool child_is_leaf>
  bool merge_root(uint16_t idx) {
    node* root = reinterpret_cast<node*>(root_);
    if constexpr (child_is_leaf) {
      if (root->child_pointers[2] == nullptr) {
        leaf_t* a_leaf = reinterpret_cast<leaf_t*>(root->child_pointers[0]);
        leaf_t* b_leaf = reinterpret_cast<leaf_t*>(root->child_pointers[1]);
        uint16_t a_size = a_leaf->size();
        uint16_t b_size = b_leaf->size();
        if (a_size + b_size <= block_size) {
          a_leaf->merge(b_leaf);
          root->child_pointers[0] = nullptr;
          root->child_pointers[1] = nullptr;
          delete (root);
          root_ = a_leaf;
          --levels_;
          return true;
        }
        a_leaf->balance(b_leaf);
        root->items[1] = b_leaf->items[0];
        return false;
      }
      leaf_t* m_leaf = reinterpret_cast<leaf_t*>(root->child_pointers[idx]);
      uint16_t m_size = m_leaf->size();
      uint16_t r_size = 0;
      leaf_t* r_leaf = nullptr;
      if (idx + 1 < block_size && root->child_pointers[idx + 1] != nullptr) {
        r_leaf = reinterpret_cast<leaf_t*>(root->child_pointers[idx + 1]);
        r_size = r_leaf->size();
      }
      uint16_t l_size = 0;
      leaf_t* l_leaf = nullptr;
      if (idx > 0) {
        l_leaf = reinterpret_cast<leaf_t*>(root->child_pointers[idx - 1]);
        l_size = l_leaf->size();
      }
      if (r_size > l_size) {
        if (r_size == block_size / 2) {
          m_leaf->merge(r_leaf);
          for (uint16_t i = idx + 2; i < block_size; ++i) {
            root->items[i - 1] = root->items[i];
            root->child_pointers[i - 1] = root->child_pointers[i];
          }
          root->items[block_size - 1] = max_value_;
          root->child_pointers[block_size - 1] = nullptr;
        } else {
          m_leaf->balance(r_leaf);
          root->items[idx + 1] = r_leaf->items[0];
        }
      } else if (l_size == block_size / 2) {
        l_leaf->merge(m_leaf);
        for (uint16_t i = idx + 1; i < block_size; ++i) {
          root->items[i - 1] = root->items[i];
          root->child_pointers[i - 1] = root->child_pointers[i];
        }
        root->items[block_size - 1] = max_value_;
        root->child_pointers[block_size - 1] = nullptr;
      } else {
        l_leaf->balance(m_leaf);
        root->items[idx] = m_leaf->items[0];
      }
      return false;
    } else {
      if (root->child_pointers[2] == nullptr) {
        node* a_node = reinterpret_cast<node*>(root->child_pointers[0]);
        node* b_node = reinterpret_cast<node*>(root->child_pointers[1]);
        uint16_t a_size = a_node->size();
        uint16_t b_size = b_node->size();
        if (a_size + b_size <= block_size) {
          a_node->merge(b_node, a_size, b_size);
          root->child_pointers[0] = nullptr;
          root->child_pointers[1] = nullptr;
          delete (root);
          root_ = a_node;
          --levels_;
          return true;
        }
        a_node->balance(b_node, a_size, b_size);
        root->items[1] = b_node->items[0];
        return false;
      }
      node* m_node = reinterpret_cast<node*>(root->child_pointers[idx]);
      uint16_t m_size = m_node->size();
      uint16_t r_size = 0;
      node* r_node = nullptr;
      if (idx + 1 < block_size && root->child_pointers[idx + 1] != nullptr) {
        r_node = reinterpret_cast<node*>(root->child_pointers[idx + 1]);
        r_size = r_node->size();
      }
      uint16_t l_size = 0;
      node* l_node = nullptr;
      if (idx > 0) {
        l_node = reinterpret_cast<node*>(root->child_pointers[idx - 1]);
        l_size = l_node->size();
      }
      if (r_size > l_size) {
        if (r_size == block_size / 2) {
          m_node->merge(r_node, m_size, r_size);
          for (uint16_t i = idx + 2; i < block_size; ++i) {
            root->items[i - 1] = root->items[i];
            root->child_pointers[i - 1] = root->child_pointers[i];
          }
          root->items[block_size - 1] = max_value_;
          root->child_pointers[block_size - 1] = nullptr;
        } else {
          m_node->balance(r_node, m_size, r_size);
          root->items[idx + 1] = r_node->items[0];
        }
      } else if (l_size == block_size / 2) {
        l_node->merge(m_node, l_size, m_size);
        for (uint16_t i = idx + 1; i < block_size; ++i) {
          root->items[i - 1] = root->items[i];
          root->child_pointers[i - 1] = root->child_pointers[i];
        }
        root->items[block_size - 1] = max_value_;
        root->child_pointers[block_size - 1] = nullptr;
      } else {
        l_node->balance(m_node, l_size, m_size);
        root->items[idx] = m_node->items[0];
      }
      return false;
    }
  }

 public:
  dynamic_tree(const T& mv) : max_value_(mv), levels_(0) {
    root_ = new leaf_t(mv);
  }

  ~dynamic_tree() {
    if (root_ != nullptr) {
      if (levels_) {
        std::vector<std::pair<node*, uint16_t>> node_stack;
        node_stack.push_back({reinterpret_cast<node*>(root_), 0});
        while (node_stack.size() < levels_) {
          auto p = node_stack.back();
          node_stack.push_back(
              {reinterpret_cast<node*>(p.first->child_pointers[p.second]), 0});
        }
        while (node_stack.size() > 0) {
          auto p = node_stack.back();
          node* c_node = p.first;
          for (uint16_t i = 0; i < block_size; ++i) {
            if (c_node->child_pointers[i] == nullptr) {
              break;
            }
            leaf_t* lefa = reinterpret_cast<leaf_t*>(c_node->child_pointers[i]);
            delete (lefa);
          }
          delete (c_node);
          node_stack.pop_back();
          while (node_stack.size() > 0) {
            auto n_p = node_stack.back();
            node_stack.pop_back();
            uint16_t idx = n_p.second + 1;
            if (idx >= block_size ||
                n_p.first->child_pointers[idx] == nullptr) {
              delete (n_p.first);
            } else {
              node_stack.push_back({n_p.first, idx});
              while (node_stack.size() < levels_) {
                n_p = node_stack.back();
                node_stack.push_back(
                    {reinterpret_cast<node*>(
                         n_p.first->child_pointers[n_p.second]),
                     0});
              }
              break;
            }
          }
        }

      } else {
        leaf_t* root = reinterpret_cast<leaf_t*>(root_);
        delete (root);
      }
    }
  }

  bt_iterator begin() const {
    if (levels_ == 0) {
      return {reinterpret_cast<const leaf_t*>(root_)};
    }
    return {reinterpret_cast<const node*>(root_), levels_};
  }

  bt_iterator end() const { return {nullptr, levels_}; }

  bool read_access(const T& q, typename leaf_t::find_return_t& out) const {
    const void* nd = root_;
    for (uint16_t i = 0; i < levels_; ++i) {
      const node* n_nd = reinterpret_cast<const node*>(nd);
      auto idx = n_nd->find(q, max_value_);
      nd = n_nd->child_pointers[idx];
    }
    const leaf_t* leaf = reinterpret_cast<const leaf_t*>(nd);
    auto idx = leaf->find(q, max_value_);
    if (leaf->items[idx] != q) {
      return false;
    }
    out = leaf->get(idx);
    return true;
  }

  bt_iterator predecessor(const T& v) const {
    std::vector<std::pair<const node*, uint16_t>> node_stack;
    const void* nd = root_;
    for (uint16_t i = 0; i < levels_; ++i) {
      const node* n_nd = reinterpret_cast<const node*>(nd);
      auto idx = n_nd->find(v, max_value_);
      node_stack.push_back({n_nd, idx});
      nd = n_nd->child_pointers[idx];
    }
    const leaf_t* leaf = reinterpret_cast<const leaf_t*>(nd);
    auto idx = leaf->find(v, max_value_);
    if (leaf->items[idx] > v) {
      return end();
    }
    return {node_stack, leaf, idx, levels_};
  }

  bt_iterator lower_bound(const T& q) const {
    auto ptr = predecessor(q);
    if (ptr == end()) [[unlikely]] {
      return begin();
    }
    auto b_ptr = begin();
    while (ptr != b_ptr && ptr.key() == q) {
      --ptr;
    }
    if (ptr.key() == q) {
      return ptr;
    }
    return ++ptr;
  }

  bt_iterator upper_bound(const T& q) const {
    auto ptr = predecessor(q);
    if (ptr == end()) [[unlikely]] {
      return begin();
    }
    return ++ptr;
  }

  typename leaf_t::insert_return_t insert(const T& v) {
    void* nd = root_;
    if (levels_ == 0) {
      leaf_t* leaf = reinterpret_cast<leaf_t*>(nd);
      if (leaf->is_full()) {
        split_root<true>();
        nd = root_;
      } else {
        return leaf->insert(v, max_value_);
      }
    } else {
      node* i_nd = reinterpret_cast<node*>(nd);
      if (i_nd->is_full()) {
        split_root<false>();
        nd = root_;
      }
    }
    node* i_nd = reinterpret_cast<node*>(nd);
    leaf_t* leaf;
    for (uint16_t i = 0; i < levels_; ++i) {
      auto idx = i_nd->find(v, max_value_);
      void* c_nd = i_nd->child_pointers[idx];
      if (i + 1 < levels_) {
        node* child_node = reinterpret_cast<node*>(c_nd);
        if (child_node->is_full()) {
          i_nd->template split_child<false>(idx, max_value_);
          idx = i_nd->find(v, max_value_);
          child_node = reinterpret_cast<node*>(i_nd->child_pointers[idx]);
        }
        if (i_nd->items[idx] > v) {
          i_nd->items[idx] = v;
        }
        i_nd = child_node;
      } else {
        leaf = reinterpret_cast<leaf_t*>(c_nd);
        if (leaf->is_full()) {
          i_nd->template split_child<true>(idx, max_value_);
          idx = i_nd->find(v, max_value_);
          leaf = reinterpret_cast<leaf_t*>(i_nd->child_pointers[idx]);
        }
        if (i_nd->items[idx] > v) {
          i_nd->items[idx] = v;
        }
      }
    }
    return leaf->insert(v, max_value_);
  }

  bool remove(const T& v) {
    void* nd = root_;
    node* i_nd;
    leaf_t* leaf;
    if (levels_ == 0) {
      leaf = reinterpret_cast<leaf_t*>(nd);
      return leaf->remove(v, max_value_);
    } else if (levels_ == 1) {
      i_nd = reinterpret_cast<node*>(nd);
      auto idx = i_nd->find(v, max_value_);
      leaf = reinterpret_cast<leaf_t*>(i_nd->child_pointers[idx]);
      if (leaf->half_empty()) {
        if (merge_root<true>(idx)) {
          return reinterpret_cast<leaf_t*>(root_)->remove(v, max_value_);
        }
        idx = i_nd->find(v, max_value_);
        leaf = reinterpret_cast<leaf_t*>(i_nd->child_pointers[idx]);
      }
      bool removed = leaf->remove(v, max_value_);
      i_nd->items[idx] = leaf->items[0];
      return removed;
    } else {
      i_nd = reinterpret_cast<node*>(nd);
      auto idx = i_nd->find(v, max_value_);
      node* child = reinterpret_cast<node*>(i_nd->child_pointers[idx]);
      if (child->half_empty()) {
        if (merge_root<false>(idx)) {
          return remove(v);
        } else {
          idx = i_nd->find(v, max_value_);
          child = reinterpret_cast<node*>(i_nd->child_pointers[idx]);
        }
      }
      bool removed = child->remove(v, levels_ - 1, max_value_);
      i_nd->items[idx] = child->items[0];
      return removed;
    }
  }
};
}  // namespace internal

template <class T, bool allow_duplicates = false, uint16_t block_size = 64>
class dynamic_set {
  class leaf {
    uint16_t size_;

   public:
    typedef T* insert_return_t;
    typedef T find_return_t;
    typedef T iter_deref_t;
    const static constexpr bool leaf_allows_duplicates() {
      return allow_duplicates;
    }
    std::array<T, block_size> items;

    leaf(const T& mv) : size_(0) { items.fill(mv); }

    uint16_t size() const { return size_; }

    uint16_t find(const T& q, const T& mv) const {
      if (q == mv) [[unlikely]] {
        return size_ > 0 ? size_ - 1 : 0;
      }
      return internal::search(items, q);
    }

    bool is_full() const { return size_ == block_size; }

    bool half_empty() const { return size_ <= block_size / 2; }

    find_return_t get(uint16_t idx) const { return items[idx]; }

    leaf* split(const T& mv) {
      leaf* new_leaf = new leaf(mv);
      for (uint16_t i = 0; i < block_size / 2; ++i) {
        uint16_t src_idx = i + block_size / 2;
        new_leaf->items[i] = items[src_idx];
        items[src_idx] = mv;
      }
      new_leaf->size_ = block_size / 2;
      size_ = block_size / 2;
      return new_leaf;
    }

    insert_return_t insert(const T& v, const T& mv) {
      uint16_t idx = find(v, mv);
      if constexpr (allow_duplicates == false) {
        if (items[idx] == v && size_ > 0) {
          return nullptr;
        }
      }
      idx += items[idx] <= v;
      for (uint16_t i = block_size - 1; i > idx; --i) {
        items[i] = items[i - 1];
      }
      items[idx] = v;
      ++size_;
      return items.data() + idx;
    }

    bool remove(const T& v, const T& mv) {
      uint16_t idx = find(v, mv);
      if (items[idx] != v) {
        return false;
      }
      for (uint16_t i = idx; i < block_size - 1; ++i) {
        items[i] = items[i + 1];
      }
      items[block_size - 1] = mv;
      --size_;
      return true;
    }

    void balance(leaf* other) {
      if (size_ < other->size_) {
        T mv = items[block_size - 1];
        uint16_t copy_count = (1 + other->size_ - size_) / 2;
        for (uint16_t i = 0; i < copy_count; ++i) {
          items[size_ + i] = other->items[i];
        }
        for (uint16_t i = copy_count; i < other->size_; ++i) {
          other->items[i - copy_count] = other->items[i];
        }
        std::fill_n(other->items.data() + (other->size_ - copy_count),
                    copy_count, mv);
        size_ += copy_count;
        other->size_ -= copy_count;
      } else {
        T mv = other->items[block_size - 1];
        uint16_t copy_count = (1 + size_ - other->size_) / 2;
        for (uint16_t i = other->size_ - 1; i < block_size; --i) {
          other->items[i + copy_count] = other->items[i];
        }
        for (uint16_t i = 0; i < copy_count; ++i) {
          other->items[i] = items[size_ - copy_count + i];
          items[size_ - copy_count + i] = mv;
        }
        size_ -= copy_count;
        other->size_ += copy_count;
      }
    }

    void merge(leaf* other) {
      for (uint16_t i = 0; i < other->size_; ++i) {
        items[size_ + i] = other->items[i];
      }
      size_ += other->size_;
      delete (other);
    }
  };

  internal::dynamic_tree<T, leaf, block_size> b_tree_;
  size_t size_;

  dynamic_set& operator=(dynamic_set&) = delete;
  dynamic_set(dynamic_set&) = delete;

 public:
  dynamic_set(const T& max_value) : b_tree_(max_value), size_(0) {}

  dynamic_set() : dynamic_set(internal::max_val<T>()) {}

  dynamic_set(dynamic_set&& rhs) {
    b_tree_ = std::exchange(rhs.b_tree_, {});
    size_ = std::exchange(rhs.size_, 0);
  }

  dynamic_set& operator=(dynamic_set&& rhs) {
    b_tree_ = std::exchange(rhs.b_tree_, {});
    size_ = std::exchange(rhs.size_, 0);
  }

  auto begin() const {
    if (size_ == 0) {
      return b_tree_.end();
    }
    return b_tree_.begin();
  }

  auto end() const { return b_tree_.end(); }

  void insert(const T& v) {
    auto bla = b_tree_.insert(v);
    size_ += bla != nullptr;
  }

  bool remove(const T& v) {
    if (size_ == 0) [[unlikely]] {
      return false;
    }
    bool ret = b_tree_.remove(v);
    size_ -= ret;
    return ret;
  }

  bool contains(const T& v) const {
    if (size_ == 0) [[unlikely]] {
      return false;
    }
    typename leaf::find_return_t key;
    return b_tree_.read_access(v, key);
  }

  auto predecessor(const T& v) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.predecessor(v);
  }

  auto lower_bound(const T& v) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.lower_bound(v);
  }

  auto upper_bound(const T& v) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.upper_bound(v);
  }

  size_t count(const T& q) const {
    if constexpr (not allow_duplicates) {
      return contains(q);
    }
    return upper_bound(q) - lower_bound(q);
  }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }
};

template <class T, uint16_t block_size = 64>
using dynamic_multiset = dynamic_set<T, true, block_size>;

template <class K, class V, bool allow_duplicates = false,
          uint16_t block_size = 64>
class dynamic_map {
  class leaf {
    uint16_t size_;

   public:
    typedef V* insert_return_t;
    typedef std::pair<K, V> find_return_t;
    typedef std::pair<K, V> iter_deref_t;
    const static constexpr bool leaf_allows_duplicates() {
      return allow_duplicates;
    }
    std::array<K, block_size> items;
    std::array<V, block_size> values;

    leaf(const K& mv) : size_(0) { items.fill(mv); }

    uint16_t size() const { return size_; }

    uint16_t find(const K& q, const K& mv) const {
      if (q == mv) [[unlikely]] {
        return size_ > 0 ? size_ - 1 : 0;
      }
      return internal::search(items, q);
    }

    bool is_full() const { return size_ == block_size; }

    bool half_empty() const { return size_ <= block_size / 2; }

    find_return_t get(uint16_t idx) const { return {items[idx], values[idx]}; }

    leaf* split(const K& mv) {
      leaf* new_leaf = new leaf(mv);
      for (uint16_t i = 0; i < block_size / 2; ++i) {
        uint16_t src_idx = i + block_size / 2;
        new_leaf->items[i] = items[src_idx];
        new_leaf->values[i] = values[src_idx];
        items[src_idx] = mv;
      }
      new_leaf->size_ = block_size / 2;
      size_ = block_size / 2;
      return new_leaf;
    }

    insert_return_t insert(const K& v, const K& mv) {
      uint16_t idx = find(v, mv);
      if constexpr (allow_duplicates == false) {
        if (items[idx] == v && size_ > 0) {
          return nullptr;
        }
      }
      idx += items[idx] <= v;
      for (uint16_t i = block_size - 1; i > idx; --i) {
        items[i] = items[i - 1];
        values[i] = values[i - 1];
      }
      items[idx] = v;
      ++size_;
      return values.data() + idx;
    }

    bool remove(const K& v, const K& mv) {
      uint16_t idx = find(v, mv);
      if (items[idx] != v) {
        return false;
      }
      for (uint16_t i = idx; i < block_size - 1; ++i) {
        items[i] = items[i + 1];
        values[i] = values[i + 1];
      }
      items[block_size - 1] = mv;
      --size_;
      return true;
    }

    void balance(leaf* other) {
      if (size_ < other->size_) {
        K mv = items[block_size - 1];
        uint16_t copy_count = (1 + other->size_ - size_) / 2;
        for (uint16_t i = 0; i < copy_count; ++i) {
          items[size_ + i] = other->items[i];
          values[size_ + i] = other->values[i];
        }
        for (uint16_t i = copy_count; i < other->size_; ++i) {
          other->items[i - copy_count] = other->items[i];
          other->values[i - copy_count] = other->values[i];
        }
        std::fill_n(other->items.data() + (other->size_ - copy_count),
                    copy_count, mv);
        size_ += copy_count;
        other->size_ -= copy_count;
      } else {
        K mv = other->items[block_size - 1];
        uint16_t copy_count = (1 + size_ - other->size_) / 2;
        for (uint16_t i = other->size_ - 1; i < block_size; --i) {
          other->items[i + copy_count] = other->items[i];
          other->values[i + copy_count] = other->values[i];
        }
        for (uint16_t i = 0; i < copy_count; ++i) {
          other->items[i] = items[size_ - copy_count + i];
          other->values[i] = values[size_ - copy_count + i];
          items[size_ - copy_count + i] = mv;
        }
        size_ -= copy_count;
        other->size_ += copy_count;
      }
    }

    void merge(leaf* other) {
      for (uint16_t i = 0; i < other->size_; ++i) {
        items[size_ + i] = other->items[i];
        values[size_ + i] = other->values[i];
      }
      size_ += other->size_;
      delete (other);
    }
  };

  internal::dynamic_tree<K, leaf, block_size> b_tree_;
  size_t size_;

  dynamic_map& operator=(dynamic_map&) = delete;
  dynamic_map(dynamic_map&) = delete;

 public:
  dynamic_map(const K& max_value) : b_tree_(max_value), size_(0) {}

  dynamic_map() : dynamic_map(internal::max_val<K>()) {}

  dynamic_map(dynamic_map&& rhs) {
    b_tree_ = std::exchange(rhs.b_tree_, {});
    size_ = std::exchange(rhs.size_, 0);
  }

  dynamic_map& operator=(dynamic_map&& rhs) {
    b_tree_ = std::exchange(rhs.b_tree_, {});
    size_ = std::exchange(rhs.size_, 0);
  }

  auto begin() const {
    if (size_ == 0) {
      return b_tree_.end();
    }
    return b_tree_.begin();
  }

  auto end() const { return b_tree_.end(); }

  void insert(const K& k, const V& v) {
    auto bla = b_tree_.insert(k);
    if (bla == nullptr) {
      return;
    }
    *bla = v;
    ++size_;
  }

  void insert(const std::pair<K, V>& p) { insert(p.first, p.second); }

  bool remove(const K& k) {
    if (size_ == 0) [[unlikely]] {
      return false;
    }
    bool ret = b_tree_.remove(k);
    size_ -= ret;
    return ret;
  }

  bool contains_key(const K& k) const {
    if (size_ == 0) [[unlikely]] {
      return false;
    }
    typename leaf::find_return_t key;
    return b_tree_.read_access(k, key);
  }

  V at(const K& k) const {
    if (size_ == 0) [[unlikely]] {
      throw std::out_of_range("dynamic_map:at: Key not found");
    }
    typename leaf::find_return_t p;
    if (b_tree_.read_access(k, p)) {
      return p.second;
    }
    throw std::out_of_range("dynamic_map:at: Key not found");
  }

  auto predecessor(const K& k) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.predecessor(k);
  }

  auto lower_bound(const K& k) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.lower_bound(k);
  }

  auto upper_bound(const K& k) const {
    if (size_ == 0) [[unlikely]] {
      return end();
    }
    return b_tree_.upper_bound(k);
  }

  size_t count(const K& k) const {
    if constexpr (not allow_duplicates) {
      return contains_key(k);
    }
    return upper_bound(k) - lower_bound(k);
  }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }
};

template <class K, class V, uint16_t block_size = 64>
using dynamic_multimap = dynamic_map<K, V, true, block_size>;

}  // namespace bt
