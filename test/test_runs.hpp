#include <cstdint>
#include <map>
#include <random>
#include <set>

#include "../googletest/googletest/include/gtest/gtest.h"
#include "../include/dynamic_search.hpp"
#include "../include/static_search.hpp"
#include "util.hpp"

void run(uint64_t n, unsigned int seed) {
  run_dynamic_map<bt::dynamic_map<uint32_t, char>, std::map<uint32_t, char>>(
      n, seed);
  run_dynamic_map<bt::dynamic_multimap<uint32_t, char>,
                  std::multimap<uint32_t, char>>(n, seed);
  run_dynamic_set<bt::dynamic_set<uint32_t>, std::set<uint32_t>>(n, seed);
  run_dynamic_set<bt::dynamic_multiset<uint32_t>, std::multiset<uint32_t>>(
      n, seed);
  run_static_set<bt::static_set<uint32_t>, std::multiset<uint32_t>>(n, seed);
  run_static_map<bt::static_map<uint32_t, char>, std::multimap<uint32_t, char>>(
      n, seed);
}

TEST(Integration, AAAA) { run(100, 1337); }

TEST(Integration, AAAB) { run(1000, 1337); }

TEST(Integration, AAAC) { run(10000, 504596272); }

TEST(Integration, AAAD) { run(10000, 116668523); }

TEST(Integration, AAAE) { run(100000, 1337); }

TEST(Integration, AAAF) { run(10000, 255095582); }