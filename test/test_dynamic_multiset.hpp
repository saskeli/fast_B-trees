#pragma once

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/dynamic_search.hpp"

TEST(DynamicMultiSet, FewInserts) {
  bt::dynamic_multiset<int> set;
  set.insert(5);
  set.insert(17);
  set.insert(5);

  auto a = set.begin();
  ASSERT_EQ(*a++, 5);
  ASSERT_EQ(*a++, 5);
  ASSERT_EQ(*a++, 17);
  ASSERT_EQ(a, set.end());
  ASSERT_EQ(set.size(), 3u);
  ASSERT_TRUE(set.contains(5));
  ASSERT_TRUE(set.contains(17));
  ASSERT_FALSE(set.contains(1337));
}

TEST(DynamicMultiSet, SingleRemove) {
  bt::dynamic_multiset<int> set;
  set.insert(6);
  set.insert(6);
  set.remove(6);
  ASSERT_EQ(set.size(), 1u);
  ASSERT_EQ(*set.begin(), 6);
  ASSERT_TRUE(set.contains(6));
}

TEST(DynamicMultiSet, FP) {
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
  bt::dynamic_multiset<double> set;
  for (auto v : vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), 11u);
  ASSERT_EQ(set.count(dv), 3u);

  ASSERT_EQ(*set.predecessor(dv), dv);
  ASSERT_EQ(*set.predecessor(mv), mv);
  ASSERT_EQ(*set.predecessor(mv / 2), ff);
  ASSERT_EQ(*set.predecessor(-10), min_v);
  ASSERT_EQ(*set.lower_bound(0), n_zero);
  ASSERT_EQ(*set.upper_bound(0), ff);
}

TEST(DynamicMultiSet, Small) {
  bt::dynamic_multiset<int> set;
  for (int i = 0; i < 100; ++i) {
    set.insert(i * 5);
    set.insert(21);
  }
  ASSERT_EQ(set.count(21), 100u);
  ASSERT_EQ(*set.predecessor(23), 21);
  ASSERT_EQ(set.count(700), 0u);
  ASSERT_EQ(set.count(5), 1u);
}