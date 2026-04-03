#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/static_search.hpp"

TEST(StaticMap, Empty) {
  std::vector<std::pair<int, int>> vec;
  bt::static_map map(vec);
  auto e = map.end();
  ASSERT_EQ(map.begin(), e);
  ASSERT_EQ(map.size(), 0);

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

TEST(StaticMap, Tiny) {
  std::vector<std::pair<int, int>> vec = {{1, 5}, {4, 8}, {3, 2}};
  bt::static_map map(vec);
  auto e = map.end();

  ASSERT_EQ(map.size(), 3);

  auto a = map.begin();
  std::pair<int, int> p = {1, 5};
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

TEST(StaticMap, Small) {
  std::vector<int> k_vec = {1, 3, 4, 5, 5, 5, 5, 5, 6, 8, 7};
  std::vector<int> v_vec = {12, 11, 10, 9, 8, 7, 6, 5, 4, 2, 3};
  bt::static_map map(k_vec, v_vec);
  auto e = map.end();
  ASSERT_EQ(map.size(), 11);

  auto b = map.begin();
  auto a = map.begin();
  std::pair<int, int> p;
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

  ASSERT_EQ(map.count(5), 5);

  ASSERT_FALSE(map.contains_key(0));
  ASSERT_EQ(map.predecessor(0), e);
  p = {1, 12};
  ASSERT_EQ(*map.lower_bound(0), p);
  ASSERT_EQ(*map.upper_bound(0), p);

  ASSERT_TRUE(map.contains_key(5));
  p = {5, 9};
  ASSERT_EQ(*map.predecessor(5), p);
  p = {5, 5};
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

TEST(StaticMap, Medium) {
  std::vector<int> k_vec;
  std::vector<unsigned int> v_vec;
  for (int i = 0; i < 100; ++i) {
    v_vec.push_back(i);
    k_vec.push_back(i * i);
  }
  bt::static_map map(k_vec, v_vec);
  for (int i = 0; i <= 99 * 99; ++i) {
    ASSERT_EQ(map.contains_key(i),
              std::binary_search(k_vec.begin(), k_vec.end(), i));
  }
  auto e = map.end();
  ASSERT_EQ(map.size(), 100);

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

TEST(StaticMap, MaxV) {
  int mv = std::numeric_limits<int>::max();
  std::vector<std::pair<int, char>> vec = {{1, 'a'}, {2, 'b'}, {100, 'c'}, {mv, 'd'}, {mv, 'e'}};
  bt::static_map map(vec.data(), vec.size(), mv);
  ASSERT_EQ(map.count(mv), 2);

  ASSERT_EQ(map.upper_bound(mv) - map.lower_bound(mv), 2);
  ASSERT_EQ(map.lower_bound(mv) + 2, map.upper_bound(mv));
  ASSERT_EQ(map.upper_bound(mv) - 2, map.lower_bound(mv));

  ASSERT_EQ(map.size(), 5);
  ASSERT_TRUE(map.contains_key(mv));
  std::pair<int, char> p = {mv, 'e'};
  ASSERT_EQ(*map.predecessor(mv), p);
  p = {mv, 'd'};
  ASSERT_EQ(*map.lower_bound(mv), p);
  ASSERT_EQ(*map.upper_bound(100), p);
  ASSERT_EQ(map.upper_bound(mv), map.end());
}

