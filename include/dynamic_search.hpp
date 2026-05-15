#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "internal.hpp"

namespace bt {
namespace internal {
template <class T, class leaf_t, uint16_t block_size,
          class searcher = internal::auto_search<T, block_size>>
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
      return searcher::search(items.data(), q);
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
      typedef
          typename std::conditional<child_is_leaf, leaf_t, node>::type child_t;
      child_t* old_leaf = reinterpret_cast<child_t*>(child_pointers[idx]);
      child_t* new_leaf = old_leaf->split(mv);
      for (uint16_t i = block_size - 1; i > idx + 1; --i) {
        items[i] = items[i - 1];
        child_pointers[i] = child_pointers[i - 1];
      }
      items[idx + 1] = new_leaf->items[0];
      child_pointers[idx + 1] = new_leaf;
    }

    void merge(node* other, uint16_t size, uint16_t other_size) {
      for (uint16_t i = 0; i < other_size; ++i) {
        items[i + size] = other->items[i];
        child_pointers[i + size] = other->child_pointers[i];
        other->child_pointers[i] = nullptr;
      }
      delete (other);
    }

    template <bool has_leaves, bool balance_insertion>
    void balance(uint16_t idx, const T mv) {
      typedef typename std::conditional<has_leaves, leaf_t, node>::type child_t;
      idx += idx == 0;
      idx -= idx == block_size - 1 || child_pointers[idx + 1] == nullptr;
      assert(idx > 0);
      assert(idx < block_size - 1);
      child_t* l_child = reinterpret_cast<child_t*>(child_pointers[idx - 1]);
      uint16_t l_size = l_child->size();
      child_t* m_child = reinterpret_cast<child_t*>(child_pointers[idx]);
      uint16_t m_size = m_child->size();
      child_t* r_child = reinterpret_cast<child_t*>(child_pointers[idx + 1]);
      uint16_t r_size = r_child->size();
      uint16_t total = l_size + m_size + r_size;

      const constexpr uint16_t split_lim = 3 * block_size - 2;
      const constexpr uint16_t merge_lim = block_size + block_size / 2 + 2;
      if constexpr (balance_insertion) {
        if (total >= split_lim) {
          child_t* new_child = new child_t(items[block_size - 1]);
          l_child->four_way_split(l_size, m_child, m_size, r_child, r_size,
                                  new_child, total, mv);
          for (uint16_t i = block_size - 1; i > idx + 2; --i) {
            items[i] = items[i - 1];
            child_pointers[i] = child_pointers[i - 1];
          }
          items[idx + 2] = new_child->items[0];
          child_pointers[idx + 2] = new_child;
          items[idx + 1] = r_child->items[0];
          items[idx] = m_child->items[0];
          assert(l_child->size() >= block_size / 2);
          assert(l_child->size() <= block_size);
          assert(m_child->size() >= block_size / 2);
          assert(m_child->size() <= block_size);
          assert(r_child->size() >= block_size / 2);
          assert(r_child->size() <= block_size);
          assert(new_child->size() >= block_size / 2);
          assert(new_child->size() <= block_size);
          return;
        }
      } else {
        if (total <= merge_lim) {
          l_child->three_way_merge(l_size, m_child, m_size, r_child, r_size,
                                   total);
          delete (r_child);
          for (uint16_t i = idx + 1; i < block_size - 1; ++i) {
            items[i] = items[i + 1];
            child_pointers[i] = child_pointers[i + 1];
          }
          items[block_size - 1] = mv;
          child_pointers[block_size - 1] = nullptr;
          items[idx] = m_child->items[0];
          assert(l_child->size() >= block_size / 2);
          assert(l_child->size() <= block_size);
          assert(m_child->size() >= block_size / 2);
          assert(m_child->size() <= block_size);
          return;
        }
      }
      l_child->three_way_balance(l_size, m_child, m_size, r_child, r_size,
                                 total);
      assert(l_child->size() >= block_size / 2);
      assert(l_child->size() <= block_size);
      assert(m_child->size() >= block_size / 2);
      assert(m_child->size() <= block_size);
      assert(r_child->size() >= block_size / 2);
      assert(r_child->size() <= block_size);
      items[idx] = m_child->items[0];
      items[idx + 1] = r_child->items[0];
    }

    void four_way_split(uint16_t l_size, node* m_node, uint16_t m_size,
                        node* r_node, uint16_t r_size, node* new_node,
                        uint16_t total, const T mv) {
      uint16_t suf_size = total / 2;
      uint16_t pred_size = total - suf_size;

      uint16_t copy_count = suf_size / 2;
      uint16_t offset = r_size - copy_count;
      for (uint16_t i = 0; i < copy_count; ++i) {
        new_node->items[i] = r_node->items[offset + i];
        r_node->items[offset + i] = mv;
        new_node->child_pointers[i] = r_node->child_pointers[offset + i];
        r_node->child_pointers[offset + i] = nullptr;
      }
      suf_size -= copy_count;
      r_size = offset;

      copy_count = suf_size - r_size;
      m_node->give(m_size, copy_count, r_node, r_size, mv);
      m_size -= copy_count;

      suf_size = pred_size / 2;
      copy_count = suf_size - m_size;
      give(l_size, copy_count, m_node, m_size, mv);
    }

    void three_way_merge(uint16_t l_size, node* m_node, uint16_t m_size,
                         node* r_node, uint16_t r_size, uint16_t total) {
      const T mv = items[block_size - 1];
      uint16_t suf_size = total / 2;
      uint16_t pref_size = total - suf_size;

      uint16_t copy_count = pref_size - l_size;
      take(l_size, copy_count, m_node, m_size, mv);
      m_size -= copy_count;

      for (uint16_t i = 0; i < r_size; ++i) {
        m_node->items[m_size + i] = r_node->items[i];
        r_node->items[i] = mv;
        m_node->child_pointers[m_size + i] = r_node->child_pointers[i];
        r_node->child_pointers[i] = nullptr;
      }
    }

    void three_way_balance(uint16_t l_size, node* m_node, uint16_t m_size,
                           node* r_node, uint16_t r_size, uint16_t total) {
      const T mv = l_size < block_size
                       ? items[block_size - 1]
                       : (m_size < block_size ? m_node->items[block_size - 1]
                                              : r_node->items[block_size - 1]);
      uint16_t r_target = total / 3;
      uint16_t m_target = (total - r_target) / 2;
      uint16_t l_target = total - r_target - m_target;
      if (l_size < l_target) {
        uint16_t copy_count = l_target - l_size;
        take(l_size, copy_count, m_node, m_size, mv);
        m_size -= copy_count;
        if (m_size < m_target) {
          copy_count = m_target - m_size;
          m_node->take(m_size, copy_count, r_node, r_size, mv);
        } else if (m_size > m_target) {
          copy_count = m_size - m_target;
          m_node->give(m_size, copy_count, r_node, r_size, mv);
        }
      } else if (l_size > l_target) {
        uint16_t copy_count = l_size - l_target;
        if (block_size - m_size >= copy_count) {
          give(l_size, copy_count, m_node, m_size, mv);
          m_size += copy_count;
          if (m_size > m_target) {
            copy_count = m_size - m_target;
            m_node->give(m_size, copy_count, r_node, r_size, mv);
          } else if (m_size < m_target) {
            copy_count = m_target - m_size;
            m_node->take(m_size, copy_count, r_node, r_size, mv);
          }
        } else {
          // r_node is too small. make it the target size.
          uint16_t pre_copy = r_target - r_size;
          m_node->give(m_size, pre_copy, r_node, r_size, mv);
          m_size -= pre_copy;
          give(l_size, copy_count, m_node, m_size, mv);
        }
      } else if (m_size < m_target) {
        // l_node is correct size
        uint16_t copy_count = m_target - m_size;
        m_node->take(m_size, copy_count, r_node, r_size, mv);
      } else {
        // Some balancing was needed by definition. So (m_size > m_target) must
        // hold
        uint16_t copy_count = m_size - m_target;
        m_node->give(m_size, copy_count, r_node, r_size, mv);
      }
    }

    void take(uint16_t size, uint16_t elems, node* r_node, uint16_t r_size,
              const T& mv) {
      for (uint16_t i = 0; i < elems; ++i) {
        items[size + i] = r_node->items[i];
        r_node->items[i] = mv;
        child_pointers[size + i] = r_node->child_pointers[i];
        r_node->child_pointers[i] = nullptr;
      }
      r_size -= elems;
      for (uint16_t i = 0; i < r_size; ++i) {
        r_node->items[i] = r_node->items[elems + i];
        r_node->items[elems + i] = mv;
        r_node->child_pointers[i] = r_node->child_pointers[elems + i];
        r_node->child_pointers[elems + i] = nullptr;
      }
    }

    void give(uint16_t size, uint16_t elems, node* r_node, uint16_t r_size,
              const T& mv) {
      for (uint16_t i = r_size - 1; i < r_size; --i) {
        r_node->items[elems + i] = r_node->items[i];
        r_node->child_pointers[elems + i] = r_node->child_pointers[i];
      }
      size -= elems;
      for (uint16_t i = 0; i < elems; ++i) {
        r_node->items[i] = items[size + i];
        items[size + i] = mv;
        r_node->child_pointers[i] = child_pointers[size + i];
        child_pointers[size + i] = nullptr;
      }
    }

    bool remove(const T& v, uint16_t height, const T& mv) {
      auto idx = find(v, mv);
      if (height <= 1) {
        leaf_t* leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        if (leaf->half_empty()) {
          balance<true, false>(idx, mv);
          // merge<true>(idx, mv);
          idx = find(v, mv);
          leaf = reinterpret_cast<leaf_t*>(child_pointers[idx]);
        }
        bool removed = leaf->remove(v, mv);
        items[idx] = leaf->items[0];
        return removed;
      } else {
        node* child = reinterpret_cast<node*>(child_pointers[idx]);
        if (child->half_empty()) {
          balance<false, false>(idx, mv);
          // merge<false>(idx, mv);
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
   public:
    const leaf_t* current_leaf_;
    uint16_t current_idx_;

    bt_iterator(const bt_iterator& other)
        : current_leaf_(other.current_leaf_),
          current_idx_(other.current_idx_) {}

    bt_iterator() : current_idx_(nullptr), current_idx_(0) {}

    bt_iterator(const node* root, uint16_t levels)
        : current_leaf_(nullptr), current_idx_(0) {
      if (root == nullptr) [[unlikely]] {
        return;
      }
      const node* nd = root;
      for (uint16_t i = 0; i < levels; ++i) {
        nd = reinterpret_cast<const node*>(nd->child_pointers[0]);
      }
      current_leaf_ = reinterpret_cast<const leaf_t*>(nd);
    }

    bt_iterator(const leaf_t* leaf, uint16_t idx = 0) : current_leaf_(leaf), current_idx_(idx) {
      return;
    }

    static bt_iterator end(const node* root, uint16_t levels) {
      if (root == nullptr) [[unlikely]] {
        return {nullptr};
      }
      const node* nd = root;
      for (uint16_t i = 0; i < levels; ++i) {
        nd = reinterpret_cast<const node*>(nd->child_pointers[nd->size() - 1]);
      }
      const leaf_t* lefa = reinterpret_cast<const leaf_t*>(nd);
      return {lefa, lefa->size()};
    }

    bt_iterator& operator++() {
      ++current_idx_;
      if (current_idx_ < current_leaf_->size()) [[likely]] {
        return *this;
      }
      if (current_leaf_->right_sibling() == nullptr) [[unlikely]] {
        return *this;
      }
      current_leaf_ = current_leaf_->right_sibling();
      current_idx_ = 0;
      return *this;
    }

    bt_iterator& operator--() {
      --current_idx_;
      if (current_idx_ < block_size) {
        return *this;
      }
      current_leaf_ = current_leaf_->left_sibling();
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
      return current_leaf_ == rhs.current_leaf_ && current_idx_ == rhs.current_idx_;
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
    typedef typename std::conditional<root_is_leaf, leaf_t, node>::type child_t;
    child_t* old_root = reinterpret_cast<child_t*>(root_);
    node* new_root = new node(max_value_);
    root_ = new_root;
    new_root->items[0] = old_root->items[0];
    new_root->child_pointers[0] = old_root;
    child_t* new_leaf = old_root->split(max_value_);
    new_root->items[1] = new_leaf->items[0];
    new_root->child_pointers[1] = new_leaf;
    ++levels_;
  }

  template <bool child_is_leaf>
  bool merge_root(uint16_t idx) {
    typedef
        typename std::conditional<child_is_leaf, leaf_t, node>::type child_t;
    node* root = reinterpret_cast<node*>(root_);
    if (root->child_pointers[2] == nullptr) {
      child_t* a_child = reinterpret_cast<child_t*>(root->child_pointers[0]);
      child_t* b_child = reinterpret_cast<child_t*>(root->child_pointers[1]);
      uint16_t a_size = a_child->size();
      uint16_t b_size = b_child->size();
      if (a_size + b_size <= block_size) {
        a_child->merge(b_child, a_size, b_size);
        root->child_pointers[0] = nullptr;
        root->child_pointers[1] = nullptr;
        delete (root);
        root_ = a_child;
        --levels_;
        return true;
      }
      if (a_size > b_size) {
        uint16_t copy_count = (1 + a_size - b_size) / 2;
        a_child->give(a_size, copy_count, b_child, b_size, max_value_);
      } else {
        uint16_t copy_count = (1 + b_size - a_size) / 2;
        a_child->take(a_size, copy_count, b_child, b_size, max_value_);
      }
      root->items[1] = b_child->items[0];
      return false;
    }
    root->template balance<child_is_leaf, false>(idx, max_value_);
    return false;
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

  bt_iterator end() const { 
    if (levels_ == 0) {
      const leaf_t* lefa = reinterpret_cast<const leaf_t*>(root_);
      return {lefa, lefa->size()};
    }
    return bt_iterator::end(reinterpret_cast<const node*>(root_), levels_); 
  }

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

  const T& min_val() const {
    if (levels_ == 0) {
      return reinterpret_cast<const leaf_t*>(root_)->items[0];
    }
    return reinterpret_cast<const node*>(root_)->items[0];
  }

  bt_iterator predecessor(const T& v) const {
    if (min_val() > v) {
      return end();
    }
    const void* nd = root_;
    for (uint16_t i = 0; i < levels_; ++i) {
      const node* n_nd = reinterpret_cast<const node*>(nd);
      auto idx = n_nd->find(v, max_value_);
      nd = n_nd->child_pointers[idx];
    }
    const leaf_t* leaf = reinterpret_cast<const leaf_t*>(nd);
    auto idx = leaf->find(v, max_value_);
    return {leaf, idx};
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
    uint16_t idx;
    for (uint16_t i = 1; i < levels_; ++i) {
      idx = i_nd->find(v, max_value_);
      node* child_node = reinterpret_cast<node*>(i_nd->child_pointers[idx]);
      if (child_node->is_full()) {
        if (i == 1 && i_nd->child_pointers[2] == nullptr) [[unlikely]] {
          i_nd->template split_child<false>(idx, max_value_);
        } else {
          i_nd->template balance<false, true>(idx, max_value_);
        }
        idx = i_nd->find(v, max_value_);
        child_node = reinterpret_cast<node*>(i_nd->child_pointers[idx]);
      }
      if (i_nd->items[idx] > v) {
        i_nd->items[idx] = v;
      }
      i_nd = child_node;
    }
    idx = i_nd->find(v, max_value_);
    leaf_t* leaf = reinterpret_cast<leaf_t*>(i_nd->child_pointers[idx]);
    if (leaf->is_full()) {
      if (levels_ == 1 && i_nd->child_pointers[2] == nullptr) [[unlikely]] {
        i_nd->template split_child<true>(idx, max_value_);
      } else {
        i_nd->template balance<true, true>(idx, max_value_);
      }
      idx = i_nd->find(v, max_value_);
      leaf = reinterpret_cast<leaf_t*>(i_nd->child_pointers[idx]);
    }
    if (i_nd->items[idx] > v) {
      i_nd->items[idx] = v;
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

template <class T, bool allow_duplicates = false, uint16_t block_size = 64,
          class searcher = internal::auto_search<T, block_size>>
class dynamic_set {
  class leaf {
    leaf* left_sibling_;
    leaf* right_sibling_;
    uint16_t size_;

   public:
    typedef T* insert_return_t;
    typedef T find_return_t;
    typedef T iter_deref_t;
    static constexpr bool leaf_allows_duplicates() { return allow_duplicates; }
    std::array<T, block_size> items;

    leaf(const T& mv)
        : left_sibling_(nullptr), right_sibling_(nullptr), size_(0) {
      items.fill(mv);
    }

    leaf(const T& mv, leaf* left_sibling, leaf* right_sibling)
        : left_sibling_(left_sibling), right_sibling_(right_sibling), size_(0) {
      items.fill(mv);
    }

    const leaf* left_sibling() const { return left_sibling_; }

    const leaf* right_sibling() const { return right_sibling_; }

    uint16_t size() const { return size_; }

    uint16_t find(const T& q, const T& mv) const {
      if (q == mv) [[unlikely]] {
        return size_ > 0 ? size_ - 1 : 0;
      }
      return searcher::search(items.data(), q);
    }

    bool is_full() const { return size_ == block_size; }

    bool half_empty() const { return size_ <= block_size / 2; }

    find_return_t get(uint16_t idx) const { return items[idx]; }

    leaf* split(const T& mv) {
      leaf* new_leaf = new leaf(mv, this, right_sibling_);
      right_sibling_ = new_leaf;
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

    void four_way_split(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                        leaf* r_leaf, uint16_t r_size, leaf* new_leaf,
                        uint16_t total, const T mv) {
      uint16_t suf_size = total / 2;
      uint16_t pred_size = total - suf_size;

      uint16_t copy_count = suf_size / 2;
      uint16_t offset = r_size - copy_count;

      new_leaf->left_sibling_ = r_leaf;
      new_leaf->right_sibling_ = r_leaf->right_sibling_;
      r_leaf->right_sibling_ = new_leaf;

      for (uint16_t i = 0; i < copy_count; ++i) {
        new_leaf->items[i] = r_leaf->items[offset + i];
        r_leaf->items[offset + i] = mv;
      }
      new_leaf->size_ = copy_count;
      r_leaf->size_ = offset;
      suf_size -= copy_count;
      r_size = offset;

      copy_count = suf_size - r_size;
      m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
      m_size -= copy_count;

      suf_size = pred_size / 2;
      copy_count = suf_size - m_size;
      give(l_size, copy_count, m_leaf, m_size, mv);
    }

    void three_way_merge(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                         leaf* r_leaf, uint16_t r_size, uint16_t total) {
      const T mv = items[block_size - 1];
      uint16_t suf_size = total / 2;
      uint16_t pref_size = total - suf_size;

      uint16_t copy_count = pref_size - l_size;
      take(l_size, copy_count, m_leaf, m_size, mv);
      m_size -= copy_count;

      for (uint16_t i = 0; i < r_size; ++i) {
        m_leaf->items[m_size + i] = r_leaf->items[i];
        r_leaf->items[i] = mv;
      }
      m_leaf->size_ += r_size;

      m_leaf->right_sibling_ = r_leaf->right_sibling_;
    }

    void three_way_balance(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                           leaf* r_leaf, uint16_t r_size, uint16_t total) {
      const T mv = l_size < block_size
                       ? items[block_size - 1]
                       : (m_size < block_size ? m_leaf->items[block_size - 1]
                                              : r_leaf->items[block_size - 1]);
      uint16_t r_target = total / 3;
      uint16_t m_target = (total - r_target) / 2;
      uint16_t l_target = total - r_target - m_target;
      if (l_size < l_target) {
        uint16_t copy_count = l_target - l_size;
        take(l_size, copy_count, m_leaf, m_size, mv);
        m_size -= copy_count;
        if (m_size < m_target) {
          copy_count = m_target - m_size;
          m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
        } else if (m_size > m_target) {
          copy_count = m_size - m_target;
          m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
        }
      } else if (l_size > l_target) {
        uint16_t copy_count = l_size - l_target;
        if (block_size - m_size >= copy_count) {
          give(l_size, copy_count, m_leaf, m_size, mv);
          m_size += copy_count;
          if (m_size > m_target) {
            copy_count = m_size - m_target;
            m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
          } else if (m_size < m_target) {
            copy_count = m_target - m_size;
            m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
          }
        } else {
          // r_leaf is too small. make it the target size.
          uint16_t pre_copy = r_target - r_size;
          m_leaf->give(m_size, pre_copy, r_leaf, r_size, mv);
          m_size -= pre_copy;
          give(l_size, copy_count, m_leaf, m_size, mv);
        }
      } else if (m_size < m_target) {
        // l_leaf is correct size
        uint16_t copy_count = m_target - m_size;
        m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
      } else {
        // Some balancing was needed by definition.
        // So (m_size > m_target) must hold.
        uint16_t copy_count = m_size - m_target;
        m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
      }
    }

    void take(uint16_t size, uint16_t elems, leaf* r_leaf, uint16_t r_size,
              const T& mv) {
      for (uint16_t i = 0; i < elems; ++i) {
        items[size + i] = r_leaf->items[i];
        r_leaf->items[i] = mv;
      }
      r_size -= elems;
      for (uint16_t i = 0; i < r_size; ++i) {
        r_leaf->items[i] = r_leaf->items[elems + i];
        r_leaf->items[elems + i] = mv;
      }
      size_ = size + elems;
      r_leaf->size_ = r_size;
    }

    void give(uint16_t size, uint16_t elems, leaf* r_leaf, uint16_t r_size,
              const T& mv) {
      for (uint16_t i = r_size - 1; i < r_size; --i) {
        r_leaf->items[elems + i] = r_leaf->items[i];
      }
      size -= elems;
      for (uint16_t i = 0; i < elems; ++i) {
        r_leaf->items[i] = items[size + i];
        items[size + i] = mv;
      }
      size_ = size;
      r_leaf->size_ += elems;
    }

    void merge(leaf* other, uint16_t size, uint16_t other_size) {
      for (uint16_t i = 0; i < other_size; ++i) {
        items[size + i] = other->items[i];
      }
      size_ += other_size;
      right_sibling_ = other->right_sibling_;
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

template <class T, uint16_t block_size = 64,
          class searcher = internal::auto_search<T, block_size>>
using dynamic_multiset = dynamic_set<T, true, block_size, searcher>;

template <class K, class V, bool allow_duplicates = false,
          uint16_t block_size = 64,
          class searcher = internal::auto_search<K, block_size>>
class dynamic_map {
  class leaf {
    leaf* left_sibling_;
    leaf* right_sibling_;
    uint16_t size_;

   public:
    typedef V* insert_return_t;
    typedef std::pair<K, V> find_return_t;
    typedef std::pair<K, V> iter_deref_t;
    static constexpr bool leaf_allows_duplicates() { return allow_duplicates; }
    std::array<K, block_size> items;
    std::array<V, block_size> values;

    leaf(const K& mv, leaf* left_sibling = nullptr,
         leaf* right_sibling = nullptr)
        : left_sibling_(left_sibling), right_sibling_(right_sibling), size_(0) {
      items.fill(mv);
    }

    uint16_t size() const { return size_; }

    const leaf* left_sibling() const {
      return left_sibling_;
    }

    const leaf* right_sibling() const {
      return right_sibling_;
    }

    uint16_t find(const K& q, const K& mv) const {
      if (q == mv) [[unlikely]] {
        return size_ > 0 ? size_ - 1 : 0;
      }
      return searcher::search(items.data(), q);
    }

    bool is_full() const { return size_ == block_size; }

    bool half_empty() const { return size_ <= block_size / 2; }

    find_return_t get(uint16_t idx) const { return {items[idx], values[idx]}; }

    leaf* split(const K& mv) {
      leaf* new_leaf = new leaf(mv, this, right_sibling_);
      right_sibling_ = new_leaf;
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

    void four_way_split(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                        leaf* r_leaf, uint16_t r_size, leaf* new_leaf,
                        uint16_t total, const K mv) {
      uint16_t suf_size = total / 2;
      uint16_t pred_size = total - suf_size;

      uint16_t copy_count = suf_size / 2;
      uint16_t offset = r_size - copy_count;

      new_leaf->left_sibling_ = r_leaf;
      new_leaf->right_sibling_ = r_leaf->right_sibling_;
      r_leaf->right_sibling_ = new_leaf;

      for (uint16_t i = 0; i < copy_count; ++i) {
        new_leaf->items[i] = r_leaf->items[offset + i];
        r_leaf->items[offset + i] = mv;
        new_leaf->values[i] = r_leaf->values[offset + i];
      }
      new_leaf->size_ = copy_count;
      r_leaf->size_ = offset;
      suf_size -= copy_count;
      r_size = offset;

      copy_count = suf_size - r_size;
      m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
      m_size -= copy_count;

      suf_size = pred_size / 2;
      copy_count = suf_size - m_size;
      give(l_size, copy_count, m_leaf, m_size, mv);
    }

    void three_way_merge(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                         leaf* r_leaf, uint16_t r_size, uint16_t total) {
      const K mv = items[block_size - 1];
      uint16_t suf_size = total / 2;
      uint16_t pref_size = total - suf_size;

      uint16_t copy_count = pref_size - l_size;
      take(l_size, copy_count, m_leaf, m_size, mv);
      m_size -= copy_count;

      for (uint16_t i = 0; i < r_size; ++i) {
        m_leaf->items[m_size + i] = r_leaf->items[i];
        r_leaf->items[i] = mv;
        m_leaf->values[m_size + i] = r_leaf->values[i];
      }
      m_leaf->size_ += r_size;

      m_leaf->right_sibling_ = r_leaf->right_sibling_;
    }

    void three_way_balance(uint16_t l_size, leaf* m_leaf, uint16_t m_size,
                           leaf* r_leaf, uint16_t r_size, uint16_t total) {
      const K mv = l_size < block_size
                       ? items[block_size - 1]
                       : (m_size < block_size ? m_leaf->items[block_size - 1]
                                              : r_leaf->items[block_size - 1]);
      uint16_t r_target = total / 3;
      uint16_t m_target = (total - r_target) / 2;
      uint16_t l_target = total - r_target - m_target;
      if (l_size < l_target) {
        uint16_t copy_count = l_target - l_size;
        take(l_size, copy_count, m_leaf, m_size, mv);
        m_size -= copy_count;
        if (m_size < m_target) {
          copy_count = m_target - m_size;
          m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
        } else if (m_size > m_target) {
          copy_count = m_size - m_target;
          m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
        }
      } else if (l_size > l_target) {
        uint16_t copy_count = l_size - l_target;
        if (block_size - m_size >= copy_count) {
          give(l_size, copy_count, m_leaf, m_size, mv);
          m_size += copy_count;
          if (m_size > m_target) {
            copy_count = m_size - m_target;
            m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
          } else if (m_size < m_target) {
            copy_count = m_target - m_size;
            m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
          }
        } else {
          // r_leaf is too small. make it the target size.
          uint16_t pre_copy = r_target - r_size;
          m_leaf->give(m_size, pre_copy, r_leaf, r_size, mv);
          m_size -= pre_copy;
          give(l_size, copy_count, m_leaf, m_size, mv);
        }
      } else if (m_size < m_target) {
        // l_leaf is correct size
        uint16_t copy_count = m_target - m_size;
        m_leaf->take(m_size, copy_count, r_leaf, r_size, mv);
      } else {
        // Some balancing was needed by definition. So (m_size > m_target) must
        // hold
        uint16_t copy_count = m_size - m_target;
        m_leaf->give(m_size, copy_count, r_leaf, r_size, mv);
      }
    }

    void take(uint16_t size, uint16_t elems, leaf* r_leaf, uint16_t r_size,
              const K& mv) {
      for (uint16_t i = 0; i < elems; ++i) {
        items[size + i] = r_leaf->items[i];
        r_leaf->items[i] = mv;
        values[size + i] = r_leaf->values[i];
      }
      r_size -= elems;
      for (uint16_t i = 0; i < r_size; ++i) {
        r_leaf->items[i] = r_leaf->items[elems + i];
        r_leaf->items[elems + i] = mv;
        r_leaf->values[i] = r_leaf->values[elems + i];
      }
      size_ += elems;
      r_leaf->size_ -= elems;
    }

    void give(uint16_t size, uint16_t elems, leaf* r_leaf, uint16_t r_size,
              const K& mv) {
      for (uint16_t i = r_size - 1; i < r_size; --i) {
        r_leaf->items[elems + i] = r_leaf->items[i];
        r_leaf->values[elems + i] = r_leaf->values[i];
      }
      size -= elems;
      for (uint16_t i = 0; i < elems; ++i) {
        r_leaf->items[i] = items[size + i];
        items[size + i] = mv;
        r_leaf->values[i] = values[size + i];
      }
      size_ -= elems;
      r_leaf->size_ += elems;
    }

    void merge(leaf* other, uint16_t size, uint16_t other_size) {
      for (uint16_t i = 0; i < other_size; ++i) {
        items[size + i] = other->items[i];
        values[size + i] = other->values[i];
      }
      size_ += other_size;
      right_sibling_ = other->right_sibling_;
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

template <class K, class V, uint16_t block_size = 64,
          class searcher = internal::auto_search<K, block_size>>
using dynamic_multimap = dynamic_map<K, V, true, block_size, searcher>;

}  // namespace bt
