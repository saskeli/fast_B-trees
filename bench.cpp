#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <vector>

#include "include/dynamic_search.hpp"
#include "include/static_search.hpp"

template <class T, uint16_t block_size, class searcher>
void BM_set_insertion(benchmark::State& state) {
  bt::dynamic_set<T, false, block_size, searcher> set;
  std::mt19937_64 gen;
  std::uniform_int_distribution<T> dist;
  size_t n = state.range(0);
  const constexpr size_t insertions = 1000;
  std::vector<T> q_vec;

  while (set.size() < n - insertions / 2) {
    T val = dist(gen);
    if (set.contains(val)) {
      continue;
    }
    set.insert(val);
  }

  for (auto _ : state) {
    state.PauseTiming();
    while (q_vec.size() < insertions) {
      T val = dist(gen);
      if (set.contains(val)) {
        continue;
      }
      q_vec.push_back(val);
    }
    state.ResumeTiming();
    for (auto v : q_vec) {
      set.insert(v);
    }
    state.PauseTiming();
    for (auto v : q_vec) {
      set.remove(v);
    }
    q_vec.clear();
  }
  state.SetItemsProcessed(state.iterations() * insertions);
}

template <class T, uint16_t block_size, class searcher>
void BM_set_query(benchmark::State& state) {
  bt::dynamic_set<T, false, block_size, searcher> set;
  std::mt19937_64 gen;
  std::uniform_int_distribution<T> dist;
  size_t n = state.range(0);
  const constexpr size_t queries = 1000;
  std::vector<T> q_vec;

  while (set.size() < n) {
    T val = dist(gen);
    if (set.contains(val)) {
      continue;
    }
    set.insert(val);
    if (q_vec.size() < queries / 2) {
      q_vec.push_back(val);
    }
  }

  while (q_vec.size() < queries) {
    T val = dist(gen);
    if (set.contains(val)) {
      continue;
    }
    q_vec.push_back(val);
  }

  std::shuffle(q_vec.begin(), q_vec.end(), gen);

  uint64_t checksum = 0;
  for (auto _ : state) {
    for (auto v : q_vec) {
      checksum += set.contains(v);
    }
  }
  state.SetLabel(std::to_string(checksum));
  state.SetItemsProcessed(state.iterations() * queries);
}

template <class T, uint16_t block_size, class searcher>
void BM_flat_query(benchmark::State& state) {
  std::mt19937_64 gen;
  std::uniform_int_distribution<T> dist;
  std::vector<T> vec;
  size_t n = state.range(0);
  const constexpr size_t queries = 1000;
  std::vector<T> q_vec;

  while (vec.size() < n) {
    T val = dist(gen);
    vec.push_back(val);
    if (q_vec.size() < queries / 2) {
      q_vec.push_back(val);
    }
  }

  bt::static_set<T, block_size, searcher> set(vec);

  while (q_vec.size() < queries) {
    T val = dist(gen);
    if (set.contains(val)) {
      continue;
    }
    q_vec.push_back(val);
  }

  std::shuffle(q_vec.begin(), q_vec.end(), gen);

  uint64_t checksum = 0;
  for (auto _ : state) {
    for (auto v : q_vec) {
      checksum += set.contains(v);
    }
  }
  state.SetLabel(std::to_string(checksum));
  state.SetItemsProcessed(state.iterations() * queries);
}

#define ARGH                                                              \
  ->Arg(1000)->Arg(10000)->Arg(100000)->Arg(1000000)->Arg(10000000)->Arg( \
      100000000)

#define SET_INSERT_BM(typ, bs, st) \
  BENCHMARK(BM_set_insertion<typ, bs, bt::internal::st<typ, bs>>) ARGH;

#define SET_QUERY_BM(typ, bs, st) \
  BENCHMARK(BM_set_query<typ, bs, bt::internal::st<typ, bs>>) ARGH;

#define FLAT_QUERY_BM(typ, bs, st) \
  BENCHMARK(BM_flat_query<typ, bs, bt::internal::st<typ, bs>>) ARGH;

#define SET_BS(typ, st)       \
  SET_INSERT_BM(typ, 8, st)   \
  SET_INSERT_BM(typ, 16, st)  \
  SET_INSERT_BM(typ, 32, st)  \
  SET_INSERT_BM(typ, 64, st)  \
  SET_INSERT_BM(typ, 128, st) \
  SET_QUERY_BM(typ, 8, st)    \
  SET_QUERY_BM(typ, 16, st)   \
  SET_QUERY_BM(typ, 32, st)   \
  SET_QUERY_BM(typ, 64, st)   \
  SET_QUERY_BM(typ, 128, st)  \
  FLAT_QUERY_BM(typ, 2, st)   \
  FLAT_QUERY_BM(typ, 4, st)  \
  FLAT_QUERY_BM(typ, 8, st)  \
  FLAT_QUERY_BM(typ, 16, st)  \
  FLAT_QUERY_BM(typ, 32, st) \
  FLAT_QUERY_BM(typ, 64, st)

#define SET_QT(typ)                          \
  SET_BS(typ, linear_searcher)               \
  SET_BS(typ, templated_branchless_searcher) \
  SET_BS(typ, templated_searcher)

SET_QT(uint64_t);
SET_QT(uint32_t);

BENCHMARK_MAIN();