#include "../googletest/googletest/include/gtest/gtest.h"


struct frac {
  uint16_t numerator;
  uint16_t denominator;

  frac(uint16_t num, uint16_t den) : numerator(num), denominator(den) {}
  frac() : numerator(), denominator() {}

  bool operator>(const frac& rhs) const {
    if (denominator == rhs.denominator) {
      return numerator > rhs.numerator;
    }
    return denominator > rhs.denominator;
  }

  bool operator<(const frac& rhs) const {
    if (denominator == rhs.denominator) {
      return numerator < rhs.numerator;
    }
    return denominator < rhs.denominator;
  }

  bool operator<=(const frac& rhs) const {
    if (denominator == rhs.denominator) {
      return numerator <= rhs.numerator;
    }
    return denominator <= rhs.denominator;
  }

  bool operator>=(const frac& rhs) const {
    if (denominator == rhs.denominator) {
      return numerator >= rhs.numerator;
    }
    return denominator >= rhs.denominator;
  }

  bool operator==(const frac& rhs) const {
    return numerator == rhs.numerator && denominator == rhs.denominator;
  }

  bool operator!=(const frac& rhs) const { return !(*this == rhs); }
};

/*
TEST(Frac, comp) {
  frac a = {0, 0};
  frac b = {4, 0};
  frac c = {0, 4};
  frac d = {3, 4};

  ASSERT_FALSE(a == b);
  ASSERT_TRUE(a != b);
  ASSERT_FALSE(a == c);
  ASSERT_TRUE(a != c);

  ASSERT_TRUE(a < b);
  ASSERT_TRUE(a <= b);
  ASSERT_TRUE(a <= a);
  ASSERT_FALSE(a > b);
  ASSERT_FALSE(a >= b);
  ASSERT_TRUE(a >= a);

  ASSERT_TRUE(a < c);
  ASSERT_TRUE(a <= c);
  ASSERT_TRUE(c > a);
  ASSERT_TRUE(c >= a);

  ASSERT_TRUE(b < d);
  ASSERT_TRUE(b <= d);
  ASSERT_TRUE(d > b);
  ASSERT_TRUE(d >= b);
}*/

#include "test_static_set.hpp"

#include "test_static_map.hpp"

#include "test_dynamic_set.hpp"

#include "test_dynamic_multiset.hpp"

#include "test_dynamic_map.hpp"

#include "test_runs.hpp"