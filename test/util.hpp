#pragma once

#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#ifdef ASSERT_EQ
typedef void comp_return_t;
#else
typedef bool comp_return_t;
#endif

class randomer {
  std::mt19937_64 gen;

 public:
  randomer(unsigned int seed) : gen(seed) {}

  template <class T>
  std::vector<T> get(uint64_t n) {
    std::uniform_int_distribution<T> dist;
    std::vector<T> ret;
    for (size_t i = 0; i < n; ++i) {
      ret.push_back(dist(gen));
    }
    return ret;
  }
};

template <bool is_map = false>
comp_return_t comp(auto& ds_a, auto& ds_b) {
#ifdef ASSERT_EQ
  ASSERT_EQ(ds_a.size(), ds_b.size()) << typeid(ds_a).name();
#else
  if (ds_a.size() != ds_b.size()) {
    std::cout << "Size missmatch" << ds_a.size() << ", " << ds_b.size()
              << std::endl;
    return false;
  }
#endif

  auto a_it = ds_a.begin();
  auto a_end = ds_a.end();
  auto b_it = ds_b.begin();
  auto b_end = ds_b.end();
  while (b_it != b_end) {
#ifdef ASSERT_NE
    ASSERT_NE(a_it, a_end) << typeid(ds_a).name();
#else
    if (a_it == a_end) {
      std::cout << "Itearator ended too soon" << std::endl;
      return false;
    }
#endif

#ifdef ASSERT_EQ
    if constexpr (is_map) {
      ASSERT_EQ((*a_it).first, (*b_it).first) << typeid(ds_a).name();
    } else {
      ASSERT_EQ(*a_it, *b_it) << typeid(ds_a).name();
    }
    ++a_it;
    ++b_it;
#else
    if constexpr (is_map) {
      if ((*a_it++).first != (*b_it++).first) {
        std::cout << "Iterator missmatch" << std::endl;
        return false;
      }
    } else {
      if (*a_it++ != *b_it++) {
        std::cout << "Iterator missmatch" << std::endl;
        return false;
      }
    }
#endif
  }

#ifdef ASSERT_EQ
  ASSERT_EQ(a_it, a_end) << typeid(ds_a).name();
  ;
#else
  if (a_it != a_end) {
    std::cout << "Iterator ended too soon" << std::endl;
    return false;
  }
  return true;
#endif
}

template <class A_t, class B_t>
void run_dynamic_map(uint64_t n, unsigned int seed) {
  randomer r(seed);
  auto keys = r.get<uint32_t>(n);
  auto values = r.get<char>(n);

  A_t map_a;
  B_t map_b;

  for (uint64_t i = 0; i < n; ++i) {
    map_a.insert(keys[i], values[i]);
    map_b.insert({keys[i], values[i]});
  }

#ifdef ASSERT_EQ
  comp<true>(map_a, map_b);
#else
  if (not comp<true>(map_a, map_b)) {
    std::cout << typeid(map_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif

  for (uint64_t i = 0; i < n / 2; ++i) {
    map_b.erase(keys[i]);
    while (map_a.remove(keys[i]));
  }

#ifdef ASSERT_EQ
  comp<true>(map_a, map_b);
#else
  if (not comp<true>(map_a, map_b)) {
    std::cout << typeid(map_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif

  for (uint64_t i = n / 2; i < n; ++i) {
    map_b.erase(keys[i]);
    while (map_a.remove(keys[i]));
  }

#ifdef ASSERT_EQ
  comp<true>(map_a, map_b);
#else
  if (not comp<true>(map_a, map_b)) {
    std::cout << typeid(map_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif
}

template <class A_t, class B_t>
void run_dynamic_set(uint64_t n, unsigned int seed) {
  randomer r(seed);
  auto keys = r.get<uint32_t>(n);

  A_t set_a;
  B_t set_b;

  for (uint64_t i = 0; i < n; ++i) {
    set_a.insert(keys[i]);
    set_b.insert(keys[i]);
  }

#ifdef ASSERT_EQ
  comp(set_a, set_b);
#else
  if (not comp(set_a, set_b)) {
    std::cout << typeid(set_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif

  for (uint64_t i = 0; i < n / 2; ++i) {
    if (set_b.erase(keys[i])) {
      while (set_a.remove(keys[i]));
    }
  }

#ifdef ASSERT_EQ
  comp(set_a, set_b);
#else
  if (not comp(set_a, set_b)) {
    std::cout << typeid(set_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif

  for (uint64_t i = n / 2; i < n; ++i) {
    if (set_b.erase(keys[i])) {
      while (set_a.remove(keys[i]));
    }
  }

#ifdef ASSERT_EQ
  comp(set_a, set_b);
#else
  if (not comp(set_a, set_b)) {
    std::cout << typeid(set_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif
}

template <class A_t, class B_t>
void run_static_set(uint64_t n, unsigned int seed) {
  randomer r(seed);
  auto keys = r.get<uint32_t>(n);

  A_t set_a(keys);
  B_t set_b(keys.begin(), keys.end());

#ifdef ASSERT_EQ
  comp(set_a, set_b);
#else
  if (not comp(set_a, set_b)) {
    std::cout << typeid(set_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif
}

template <class A_t, class B_t>
void run_static_map(uint64_t n, unsigned int seed) {
  randomer r(seed);
  auto keys = r.get<uint32_t>(n);
  auto values = r.get<char>(n);

  A_t set_a(keys, values);
  B_t set_b;
  for (uint64_t i = 0; i < n; ++i) {
    set_b.insert({keys[i], values[i]});
  }

#ifdef ASSERT_EQ
  comp<true>(set_a, set_b);
#else
  if (not comp<true>(set_a, set_b)) {
    std::cout << typeid(set_a).name() << ": N = " << n << ", seed = " << seed
              << std::endl;
    exit(1);
  }
#endif
}