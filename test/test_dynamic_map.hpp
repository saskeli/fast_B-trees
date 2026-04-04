#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/static_search.hpp"

TEST(DynamicMap, Empty) {
  bt::dynamic_map<int, float> map;
  auto e = map.end();
  ASSERT_EQ(map.begin(), e);
  ASSERT_EQ(map.size(), 0u);
  ASSERT_TRUE(map.empty());

  ASSERT_THROW(map.at(12), std::out_of_range);
  ASSERT_FALSE(map.remove(12));

  ASSERT_FALSE(map.contains_key(0));
  ASSERT_EQ(map.predecessor(0), e);
  ASSERT_EQ(map.lower_bound(0), e);
  ASSERT_EQ(map.upper_bound(0), e);

  ASSERT_FALSE(map.contains_key(1));
  ASSERT_EQ(map.predecessor(1), e);
  ASSERT_EQ(map.lower_bound(1), e);
  ASSERT_EQ(map.upper_bound(1), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(map.contains_key(q));
  ASSERT_EQ(map.predecessor(q), e);
  ASSERT_EQ(map.lower_bound(q), e);
  ASSERT_EQ(map.upper_bound(q), e);
}

TEST(DynamicMap, Tiny) {
  std::vector<std::pair<int, char>> vec = {{1, 5}, {4, 8}, {3, 2}};
  bt::dynamic_map<int, char> map;
  for (auto p : vec) {
    map.insert(p);
  }

  map.insert({1, 5});

  auto e = map.end();

  ASSERT_EQ(map.size(), 3u);

  auto a = map.begin();
  std::pair<int, char> p = {1, 5};
  ASSERT_EQ(*(a++), p);
  p = {3, 2};
  ASSERT_EQ(*(a++), p);
  p = {4, 8};
  ASSERT_EQ(*(a++), p);
  ASSERT_EQ(a, e);

  ASSERT_FALSE(map.contains_key(0));
  ASSERT_EQ(map.predecessor(0), e);
  p = {1, 5};
  ASSERT_EQ(*map.lower_bound(0), p);
  ASSERT_EQ(*map.upper_bound(0), p);

  ASSERT_TRUE(map.contains_key(1));
  ASSERT_EQ(*map.predecessor(1), p);
  ASSERT_EQ(*map.lower_bound(1), p);
  p = {3, 2};
  ASSERT_EQ(*map.upper_bound(1), p);

  ASSERT_FALSE(map.contains_key(2));
  p = {1, 5};
  ASSERT_EQ(*map.predecessor(2), p);
  p = {3, 2};
  ASSERT_EQ(*map.lower_bound(2), p);
  ASSERT_EQ(*map.upper_bound(2), p);

  ASSERT_TRUE(map.contains_key(3));
  ASSERT_EQ(*map.predecessor(3), p);
  ASSERT_EQ(*map.lower_bound(3), p);
  p = {4, 8};
  ASSERT_EQ(*map.upper_bound(3), p);

  ASSERT_TRUE(map.contains_key(4));
  ASSERT_EQ(*map.predecessor(4), p);
  ASSERT_EQ(*map.lower_bound(4), p);
  ASSERT_EQ(map.upper_bound(4), e);

  ASSERT_FALSE(map.contains_key(5));
  ASSERT_EQ(*map.predecessor(5), p);
  ASSERT_EQ(map.lower_bound(5), e);
  ASSERT_EQ(map.upper_bound(5), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(map.contains_key(q));
  ASSERT_EQ(*map.predecessor(q), p);
  ASSERT_EQ(map.lower_bound(q), e);
  ASSERT_EQ(map.upper_bound(q), e);
}

TEST(DynamicMap, Medium) {
  bt::dynamic_map<int, unsigned int> map;
  std::vector<int> k_vec;
  std::vector<unsigned int> v_vec;
  for (int i = 0; i < 100; ++i) {
    v_vec.push_back(i);
    k_vec.push_back(i * i);
    map.insert(i * i, i);
  }

  for (int i = 0; i <= 99 * 99; ++i) {
    ASSERT_EQ(map.contains_key(i),
              std::binary_search(k_vec.begin(), k_vec.end(), i));
  }
  auto e = map.end();
  ASSERT_EQ(map.size(), 100u);

  std::pair<int, unsigned int> p;
  for (int i = 0; i < 100; ++i) {
    int next = i + 1;
    next *= next;
    for (int q = i * i; q < next; ++q) {
      p = {i * i, i};
      ASSERT_EQ(*map.predecessor(q), p);
      if (i < 99) {
        p = {next, i + 1};
        ASSERT_EQ(*map.upper_bound(q), p);
      } else {
        ASSERT_EQ(map.upper_bound(q), e);
      }
    }
  }
  for (int i = 0; i < 99; ++i) {
    int next = i + 1;
    next *= next;
    p = {i * i, i};
    ASSERT_EQ(*map.lower_bound(i * i), p);
    for (int q = i * i + 1; q < next; ++q) {
      p = {next, i + 1};
      ASSERT_EQ(*map.lower_bound(q), p);
    }
  }
  p = {99 * 99, 99};
  ASSERT_EQ(*map.lower_bound(99 * 99), p);
  for (int q = 99 * 99 + 1; q < 100 * 100; ++q) {
    ASSERT_EQ(map.lower_bound(q), e);
  }
}

TEST(DynamicMultiMap, Tiny) {
  std::vector<int> k_vec = {1, 3, 4, 5, 5, 5, 5, 5, 6, 8, 7};
  std::vector<long> v_vec = {12, 11, 10, 9, 8, 7, 6, 5, 4, 2, 3};
  bt::dynamic_multimap<int, long> map;

  for (size_t i = 0; i < k_vec.size(); ++i) {
    map.insert(k_vec[i], v_vec[i]);
  }
  auto e = map.end();
  ASSERT_EQ(map.size(), 11u);

  auto b = map.begin();
  auto a = map.begin();
  std::pair<int, long> p;
  while (a != e) {
    auto dist = a - b;
    int cmp = k_vec[dist];
    if (cmp == 8) {
      cmp = 7;
    } else if (cmp == 7) {
      cmp = 8;
    }
    ASSERT_EQ((*a).first, cmp);
    ++a;
  }
  ASSERT_EQ(a, e);

  ASSERT_EQ(map.count(5), 5u);

  ASSERT_FALSE(map.contains_key(0));
  ASSERT_EQ(map.predecessor(0), e);
  p = {1, 12};
  ASSERT_EQ(*map.lower_bound(0), p);
  ASSERT_EQ(*map.upper_bound(0), p);

  ASSERT_TRUE(map.contains_key(5));
  p = {5, 5};
  ASSERT_EQ(*map.predecessor(5), p);
  p = {5, 9};
  ASSERT_EQ(*map.lower_bound(5), p);
  p = {6, 4};
  ASSERT_EQ(*map.upper_bound(5), p);

  ASSERT_TRUE(map.contains_key(8));
  p = {8, 2};
  ASSERT_EQ(*map.predecessor(8), p);
  ASSERT_EQ(*map.lower_bound(8), p);
  ASSERT_EQ(map.upper_bound(8), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(map.contains_key(q));
  ASSERT_EQ(*map.predecessor(q), p);
  ASSERT_EQ(map.lower_bound(q), e);
  ASSERT_EQ(map.upper_bound(q), e);
}

TEST(DynamicMultiMap, MaxV) {
  int mv = std::numeric_limits<int>::max();
  std::vector<std::pair<int, char>> vec = {
      {1, 'a'}, {2, 'b'}, {100, 'c'}, {mv, 'd'}, {mv, 'e'}};

  bt::dynamic_multimap<int, char> map(mv);
  for (auto p : vec) {
    map.insert(p);
  }

  ASSERT_EQ(map.count(mv), 2u);

  ASSERT_EQ(map.upper_bound(mv) - map.lower_bound(mv), 2u);
  ASSERT_EQ(map.lower_bound(mv) + 2, map.upper_bound(mv));

  ASSERT_EQ(map.size(), 5u);
  ASSERT_TRUE(map.contains_key(mv));
  std::pair<int, char> p = {mv, 'e'};
  ASSERT_EQ(*map.predecessor(mv), p);
  p = {mv, 'd'};
  ASSERT_EQ(*map.lower_bound(mv), p);
  ASSERT_EQ(*map.upper_bound(100), p);
  ASSERT_EQ(map.upper_bound(mv), map.end());
}

TEST(DynamicMultiMap, Medium) {
  const char mv = 'f';
  bt::dynamic_multimap<char, int> map(mv);

  for (int i = -100; i < 100; ++i) {
    map.insert('a', i);
    map.insert('c', i);
    map.insert('e', i);
    map.insert('f', i);
  }
  map.insert('b', 12);

  ASSERT_EQ(map.count('a'), 200u);
  ASSERT_EQ(map.at('b'), 12);

  ASSERT_THROW(map.at('d'), std::out_of_range);

  for (int i = 0; i < 200; ++i) {
    ASSERT_TRUE(map.remove('a'));
  }
  ASSERT_FALSE(map.remove('a'));
  ASSERT_TRUE(map.remove('c'));

  ASSERT_EQ(map.size(), 600u);

  for (int i = 0; i < 200; ++i) {
    ASSERT_TRUE(map.remove('e'));
    ASSERT_TRUE(map.remove('f'));
  }
  ASSERT_FALSE(map.remove('e'));
  ASSERT_FALSE(map.remove('f'));
}


TEST(DynamicMap, SufficientRandom) {
  const uint64_t n = 200000;
  uint64_t i;
  std::vector<std::pair<uint64_t, int>> vec;
  for (i = 0; i < n; ++i) {
    vec.push_back({i, int(i) - int(n)});
  }
  std::vector<std::pair<uint64_t, int>> s_vec(vec);
  std::mt19937_64 rand(1337);
  std::shuffle(s_vec.begin(), s_vec.end(), rand);

  bt::dynamic_map<uint64_t, int> set;

  for (auto v : s_vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), n);
  for (auto v : vec) {
    ASSERT_TRUE(set.contains_key(v.first)) << i;
  }

  i = 0;
  for (auto v : set) {
    ASSERT_EQ(v.first, i++);
  }

  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.remove(s_vec[i].first)) << i;
    ASSERT_EQ(set.size(), size_t(n - 1 - i));
  }

  std::sort(s_vec.data() + (n / 2), s_vec.data() + s_vec.size());

  std::pair<uint64_t, int>* ptr = s_vec.data() + (n / 2);
  for (auto v : set) {
    ASSERT_EQ(v, *(ptr++));
  }

  ptr = s_vec.data() + (n / 2);
  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.contains_key(ptr[i].first));
  }
}
