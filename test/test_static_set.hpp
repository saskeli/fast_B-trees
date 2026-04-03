#pragma once

#include <algorithm>
#include <limits>
#include <vector>

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/static_search.hpp"

TEST(StaticSet, Empty) {
  std::vector<int> vec;
  bt::static_set set(vec);
  auto e = set.end();
  ASSERT_EQ(set.begin(), e);
  ASSERT_EQ(set.size(), 0);

  ASSERT_FALSE(set.contains(0));
  ASSERT_EQ(set.predecessor(0), e);
  ASSERT_EQ(set.lower_bound(0), e);
  ASSERT_EQ(set.upper_bound(0), e);

  ASSERT_FALSE(set.contains(1));
  ASSERT_EQ(set.predecessor(1), e);
  ASSERT_EQ(set.lower_bound(1), e);
  ASSERT_EQ(set.upper_bound(1), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(set.contains(q));
  ASSERT_EQ(set.predecessor(q), e);
  ASSERT_EQ(set.lower_bound(q), e);
  ASSERT_EQ(set.upper_bound(q), e);
}

TEST(StaticSet, Tiny) {
  std::vector<int> vec = {3, 1, 4};
  bt::static_set set(vec);
  auto e = set.end();
  ASSERT_EQ(set.size(), 3);

  ASSERT_FALSE(set.contains(0));
  ASSERT_EQ(set.predecessor(0), e);
  ASSERT_EQ(*set.lower_bound(0), 1);
  ASSERT_EQ(*set.upper_bound(0), 1);

  ASSERT_TRUE(set.contains(1));
  ASSERT_EQ(*set.predecessor(1), 1);
  ASSERT_EQ(*set.lower_bound(1), 1);
  ASSERT_EQ(*set.upper_bound(1), 3);

  ASSERT_FALSE(set.contains(2));
  ASSERT_EQ(*set.predecessor(2), 1);
  ASSERT_EQ(*set.lower_bound(2), 3);
  ASSERT_EQ(*set.upper_bound(2), 3);

  ASSERT_TRUE(set.contains(3));
  ASSERT_EQ(*set.predecessor(3), 3);
  ASSERT_EQ(*set.lower_bound(3), 3);
  ASSERT_EQ(*set.upper_bound(3), 4);

  ASSERT_TRUE(set.contains(4));
  ASSERT_EQ(*set.predecessor(4), 4);
  ASSERT_EQ(*set.lower_bound(4), 4);
  ASSERT_EQ(set.upper_bound(4), e);

  ASSERT_FALSE(set.contains(5));
  ASSERT_EQ(*set.predecessor(5), 4);
  ASSERT_EQ(set.lower_bound(5), e);
  ASSERT_EQ(set.upper_bound(5), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(set.contains(q));
  ASSERT_EQ(*set.predecessor(q), 4);
  ASSERT_EQ(set.lower_bound(q), e);
  ASSERT_EQ(set.upper_bound(q), e);
}

TEST(StaticSet, Small) {
  std::vector<int> vec = {1, 3, 4, 5, 5, 5, 5, 5, 6, 7, 8};
  bt::static_set set(vec);
  auto e = set.end();
  ASSERT_EQ(set.size(), 11);
  ASSERT_EQ(set.count(5), 5);

  ASSERT_FALSE(set.contains(0));
  ASSERT_EQ(set.predecessor(0), e);
  ASSERT_EQ(*set.lower_bound(0), 1);
  ASSERT_EQ(*set.upper_bound(0), 1);

  ASSERT_TRUE(set.contains(5));
  ASSERT_EQ(*set.predecessor(5), 5);
  ASSERT_EQ(*set.lower_bound(5), 5);
  ASSERT_EQ(*set.upper_bound(5), 6);

  ASSERT_TRUE(set.contains(8));
  ASSERT_EQ(*set.predecessor(8), 8);
  ASSERT_EQ(*set.lower_bound(8), 8);
  ASSERT_EQ(set.upper_bound(8), e);

  int q = std::numeric_limits<int>::max();
  ASSERT_FALSE(set.contains(q));
  ASSERT_EQ(*set.predecessor(q), 8);
  ASSERT_EQ(set.lower_bound(q), e);
  ASSERT_EQ(set.upper_bound(q), e);
}

TEST(StaticSet, Medium) {
  std::vector<int> vec;
  for (int i = 0; i < 100; ++i) {
    vec.push_back(i * i);
  }
  bt::static_set set(vec);
  for (int i = 0; i <= 99 * 99; ++i) {
    ASSERT_EQ(set.contains(i), std::binary_search(vec.begin(), vec.end(), i));
  }
  auto e = set.end();
  ASSERT_EQ(set.size(), 100);

  for (int i = 0; i < 100; ++i) {
    int next = i + 1;
    next *= next;
    for (int q = i * i; q < next; ++q) {
      ASSERT_EQ(*set.predecessor(q), i * i);
      if (i < 99) {
        ASSERT_EQ(*set.upper_bound(q), next);
      } else {
        ASSERT_EQ(set.upper_bound(q), e);
      }
    }
  }
  for (int i = 0; i < 99; ++i) {
    int next = i + 1;
    next *= next;
    ASSERT_EQ(*set.lower_bound(i * i), i * i);
    for (int q = i * i + 1; q < next; ++q) {
      ASSERT_EQ(*set.lower_bound(q), next);
    }
  }
  ASSERT_EQ(*set.lower_bound(99 * 99), 99 * 99);
  for (int q = 99 * 99 + 1; q < 100 * 100; ++q) {
    ASSERT_EQ(set.lower_bound(q), e);
  }
}

TEST(StaticSet, MaxV) {
  int mv = std::numeric_limits<int>::max();
  std::vector<int> vec = {1, 2, 100, mv, mv};
  bt::static_set set(vec);
  ASSERT_EQ(set.count(mv), 2);
  ASSERT_EQ(set.size(), 5);
  ASSERT_TRUE(set.contains(mv));
  ASSERT_EQ(*set.predecessor(mv), mv);
  ASSERT_EQ(*set.lower_bound(mv), mv);
  ASSERT_EQ(*set.upper_bound(100), mv);
  ASSERT_EQ(set.upper_bound(mv), set.end());
}

TEST(StaticSet, FP) {
  double dv = std::numeric_limits<double>::infinity();
  double mv = std::numeric_limits<double>::max();
  double ff = 44.6;
  double zero = 0;
  double n_zero = zero * -1;
  double m_f = -5;
  double min_v = std::numeric_limits<double>::lowest();
  double n_inf = dv * -1;
  std::vector<double> vec = {n_inf, min_v, m_f, n_zero, zero, ff,
                             mv,    mv,    dv,  dv,     dv};
  bt::static_set t_set(vec);
  bt::static_set set = t_set;

  ASSERT_EQ(set.size(), 11);
  ASSERT_EQ(set.count(dv), 3);

  ASSERT_EQ(*set.predecessor(dv), dv);
  ASSERT_EQ(*set.predecessor(mv), mv);
  ASSERT_EQ(*set.predecessor(mv / 2), ff);
  ASSERT_EQ(*set.predecessor(-10), min_v);
  ASSERT_EQ(*set.lower_bound(0), n_zero);
  ASSERT_EQ(*set.upper_bound(0), ff);
  ASSERT_EQ(set.upper_bound(0) - set.lower_bound(0), 2);
}

TEST(StaticSet, CustomType) {
  frac mv = {std::numeric_limits<uint16_t>::max(),
             std::numeric_limits<uint16_t>::max()};
  std::vector<frac> vec = {{1, 1},  {0, 1},  {4, 4},     {1, 15}, {23, 4},
                           {40, 0}, {0, 40}, {100, 100}, {4, 4},  {15, 15}};
  bt::static_set set(vec, mv);
  ASSERT_EQ(set.contains({20, 20}), false);

  auto a = set.lower_bound({0, 0});
  auto b = set.upper_bound({0, 2});

  ASSERT_EQ(b - a, 3);

  ASSERT_EQ(*a, frac(40, 0));
  ++a;
  ASSERT_EQ(*a, frac(0, 1));
  ++a;
  ASSERT_EQ(*a, frac(1, 1));
  ++a;
  ASSERT_EQ(a, b);
}