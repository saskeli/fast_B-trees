#include <random>
#include <map>
#include <set>

#include "test/util.hpp"

#include "include/dynamic_search.hpp"
#include "include/static_search.hpp"

int main(int argc, char const* argv[]) {
  uint64_t n = 1000000;
  if (argc > 1) {
    n = std::stoull(argv[1]);
  }
  std::random_device rd;
  for (size_t i = 1; i > 0; ++i) {
    unsigned int seed = rd();
    std::cout << "Running " << seed << "..." << std::flush;
    run_dynamic_map<bt::dynamic_map<uint32_t, char>, std::map<uint32_t, char>>(
        n, seed);
    run_dynamic_map<bt::dynamic_multimap<uint32_t, char>,
                    std::multimap<uint32_t, char>>(n, seed);
    run_dynamic_set<bt::dynamic_set<uint32_t>, std::set<uint32_t>>(n, seed);
    run_dynamic_set<bt::dynamic_multiset<uint32_t>, std::multiset<uint32_t>>(
        n, seed);
    run_static_set<bt::static_set<uint32_t>, std::multiset<uint32_t>>(n, seed);
    run_static_map<bt::static_map<uint32_t, char>,
                   std::multimap<uint32_t, char>>(n, seed);
    std::cout << "Completed " << i << "\r" << std::flush;
  }
  return 0;
}
