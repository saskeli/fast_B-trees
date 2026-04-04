#pragma once

#include <algorithm>
#include <random>

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/dynamic_search.hpp"

TEST(DynamicSet, Empty) {
  bt::dynamic_set<int> set;
  auto b = set.begin();
  auto e = set.end();
  ASSERT_EQ(b, e);
  ASSERT_EQ(set.size(), 0u);
  ASSERT_TRUE(set.empty());

  ASSERT_FALSE(set.remove(121));
  
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

TEST(DynamicSet, SingleInsert) {
  bt::dynamic_set<int> set;
  set.insert(5);
  auto b = set.begin();
  ASSERT_EQ(*b, 5);
  ++b;
  ASSERT_EQ(b, set.end());
  ASSERT_EQ(set.size(), 1u);
  ASSERT_TRUE(set.contains(5));
  ASSERT_FALSE(set.contains(1));
}

TEST(DynamicSet, FewInserts) {
  bt::dynamic_set<int> set;
  set.insert(5);
  set.insert(17);
  set.insert(5);
  ASSERT_EQ(set.size(), 2u);
  auto a = set.begin();
  ASSERT_EQ(*a, 5);
  a++;
  ASSERT_EQ(*a, 17);
  a++;
  ASSERT_EQ(a, set.end());

  ASSERT_TRUE(set.contains(5));
  ASSERT_TRUE(set.contains(17));
  ASSERT_FALSE(set.contains(1337));
}

TEST(DynamicSet, SingleRemove) {
  bt::dynamic_set<int> set;
  set.insert(6);
  set.remove(6);
  ASSERT_EQ(set.size(), 0u);
  ASSERT_EQ(set.begin(), set.end());
  ASSERT_FALSE(set.contains(6));
}

TEST(DynamicSet, Tiny) {
  bt::dynamic_set<int> set;
  set.insert(3);
  set.insert(1);
  set.insert(4);
  auto e = set.end();
  ASSERT_EQ(set.size(), 3u);
  ASSERT_FALSE(set.remove(121));

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

TEST(DynamicSet, Small) {
  std::vector<uint64_t> vec;
  for (uint64_t i = 0; i < 100; ++i) {
    vec.push_back(i);
  }
  std::vector<uint64_t> s_vec(vec);
  std::mt19937_64 rand(1337);
  std::shuffle(s_vec.begin(), s_vec.end(), rand);

  bt::dynamic_set<uint64_t> set;
  for (auto v : s_vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), vec.size());
  for (auto v : vec) {
    ASSERT_TRUE(set.contains(v));
  }

  uint64_t i = 0;
  for (auto v : set) {
    ASSERT_EQ(v, vec[i++]);
  }

  for (i = 0; i < 50; ++i) {
    set.remove(s_vec[i]);
  }
  ASSERT_EQ(set.size(), 50u);
  std::sort(s_vec.data() + 50, s_vec.data() + 100);
  i = 50;
  for (auto v : set) {
    ASSERT_EQ(v, s_vec[i++]);
  }
}

TEST(DynamicSet, MediumOrdered) {
  bt::dynamic_set<uint64_t> set;
  uint64_t i;
  for (i = 0; i < 5000; ++i) {
    set.insert(i);
  }

  ASSERT_EQ(set.size(), 5000u);
  for (i = 0; i < 5000; ++i) {
    ASSERT_TRUE(set.contains(i)) << i;
  }

  i = 0;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = 0; i < 2500; ++i) {
    ASSERT_TRUE(set.remove(i)) << i;
    ASSERT_EQ(set.size(), size_t(5000 - i - 1));
  }
  i = 2500;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }
}

TEST(DynamicSet, MediumReversed) {
  bt::dynamic_set<uint64_t> set;
  const uint64_t n = 5000;  // (2049 + 2050) / 2
  uint64_t i;
  for (i = n; i > 0; --i) {
    set.insert(i);
  }

  ASSERT_EQ(set.size(), n);
  for (i = 1; i <= n; ++i) {
    ASSERT_TRUE(set.contains(i)) << i;
  }
  i = 1;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = n / 2; i > 0; --i) {
    ASSERT_TRUE(set.remove(i)) << i;
    ASSERT_EQ(set.size(), size_t(n / 2 - 1 + i));
  }
  i = n / 2 + 1;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }
}

TEST(DynamicSet, MediumRandom) {
  const uint64_t n = 5000;
  uint64_t i;
  std::vector<uint64_t> vec;
  for (i = 0; i < n; ++i) {
    vec.push_back(i);
  }
  std::vector<uint64_t> s_vec(vec);
  std::mt19937_64 rand(1337);
  std::shuffle(s_vec.begin(), s_vec.end(), rand);

  bt::dynamic_set<uint64_t> set;

  for (auto v : s_vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), n);
  for (auto v : vec) {
    ASSERT_TRUE(set.contains(v)) << i;
  }

  i = 0;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.remove(s_vec[i])) << i;
    ASSERT_EQ(set.size(), size_t(n - 1 - i));
  }

  std::sort(s_vec.data() + (n / 2), s_vec.data() + s_vec.size());

  uint64_t* ptr = s_vec.data() + (n / 2);
  for (auto v : set) {
    ASSERT_EQ(v, *(ptr++));
  }

  ptr = s_vec.data() + (n / 2);
  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.contains(ptr[i]));
  }
}

TEST(DynamicSet, SufficientOrdered) {
  const uint64_t n = 200000;
  bt::dynamic_set<uint64_t> set;
  uint64_t i;
  for (i = 0; i < n; ++i) {
    set.insert(i);
  }

  ASSERT_EQ(set.size(), n);
  for (i = 0; i < n; ++i) {
    ASSERT_TRUE(set.contains(i)) << i;
  }

  i = 0;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.remove(i)) << i;
    ASSERT_EQ(set.size(), size_t(n - i - 1));
  }
  i = n / 2;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }
}

TEST(DynamicSet, SufficientReversed) {
  bt::dynamic_set<uint64_t> set;
  const uint64_t n = 200000;
  uint64_t i;
  for (i = n; i > 0; --i) {
    set.insert(i);
  }

  ASSERT_EQ(set.size(), n);
  for (i = 1; i <= n; ++i) {
    ASSERT_TRUE(set.contains(i)) << i;
  }
  i = 1;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = n / 2; i > 0; --i) {
    ASSERT_TRUE(set.remove(i)) << i;
    ASSERT_EQ(set.size(), size_t(n / 2 - 1 + i));
  }
  i = n / 2 + 1;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }
}

TEST(DynamicSet, SufficientRandom) {
  const uint64_t n = 200000;
  uint64_t i;
  std::vector<uint64_t> vec;
  for (i = 0; i < n; ++i) {
    vec.push_back(i);
  }
  std::vector<uint64_t> s_vec(vec);
  std::mt19937_64 rand(1337);
  std::shuffle(s_vec.begin(), s_vec.end(), rand);

  bt::dynamic_set<uint64_t> set;

  for (auto v : s_vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), n);
  for (auto v : vec) {
    ASSERT_TRUE(set.contains(v)) << i;
  }

  i = 0;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }

  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.remove(s_vec[i])) << i;
    ASSERT_EQ(set.size(), size_t(n - 1 - i));
  }

  std::sort(s_vec.data() + (n / 2), s_vec.data() + s_vec.size());

  uint64_t* ptr = s_vec.data() + (n / 2);
  for (auto v : set) {
    ASSERT_EQ(v, *(ptr++));
  }

  ptr = s_vec.data() + (n / 2);
  for (i = 0; i < n / 2; ++i) {
    ASSERT_TRUE(set.contains(ptr[i]));
  }
}

TEST(DynamicSet, MaxVal) {
  bt::dynamic_set<uint16_t> set(500);
  uint16_t i;
  for (i = 1; i <= 500; ++i) {
    set.insert(i);
  }
  ASSERT_EQ(set.size(), 500u);

  for (i = 1; i < 500; ++i) {
    ASSERT_TRUE(set.contains(i));
  }

  i = 1;
  for (auto v : set) {
    ASSERT_EQ(v, i++);
  }
}

TEST(DynamicSet, FP) {
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
  bt::dynamic_set<double> set;
  for (auto v : vec) {
    set.insert(v);
  }

  ASSERT_EQ(set.size(), 7u);
  ASSERT_EQ(set.count(dv), 1u);

  ASSERT_EQ(*set.predecessor(dv), dv);
  ASSERT_EQ(*set.predecessor(mv), mv);
  ASSERT_EQ(*set.predecessor(mv / 2), ff);
  ASSERT_EQ(*set.predecessor(-10), min_v);
  ASSERT_EQ(*set.lower_bound(0), n_zero);
  ASSERT_EQ(*set.upper_bound(0), ff);
}