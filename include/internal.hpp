#pragma once

#include <limits>
#include <type_traits>

namespace bt::internal {
template <class T, class index_t, index_t size>
index_t linear_scan_cmov(const T* arr, const T q) {
  static_assert(__builtin_popcount(size) == 1);
  static_assert(size >= 2);
  index_t res = 1;
  for (index_t idx = 1; idx < size; idx++) {
    res += arr[idx] <= q;
  }
  return res - 1;
}

template <class T, class index_t, index_t size>
index_t templated_cmov(const T* arr, const T q) {
  static_assert(__builtin_popcountll(size) == 1);
  static_assert(size >= 2);
  if constexpr (size == 2) {
    return arr[1] <= q;
  } else {
    index_t offset = (arr[size / 2] <= q) * (size / 2);
    index_t res = templated_cmov<T, index_t, size / 2>(arr + offset, q);
    return res + offset;
  }
}

template <class T, class index_t, index_t size>
inline index_t templated_binary(const T* arr, const T q) {
  static_assert(__builtin_popcountll(size) == 1);
  static_assert(size >= 2);
  if constexpr (size == 2) {
    if (arr[1] > q) {
      return 0;
    } else {
      return 1;
    }
  } else {
    if (arr[size / 2] > q) {
      return templated_binary<T, index_t, size / 2>(arr, q);
    } else {
      return size / 2 +
             templated_binary<T, index_t, size / 2>(arr + size / 2, q);
    }
  }
}

template <class T, class index_t, index_t size>
index_t search(const T* arr, const T q) {
#if defined(__aarch64__) || defined(__arm__)
  if constexpr (sizeof(T) <= 1) {
    if constexpr (size >= 256) {
      return templated_binary<T, index_t, size>(arr, q);
    } else {
      return linear_scan_cmov<T, index_t, size>(arr, q);
    }
  } else {
    if constexpr (size >= 256) {
      return templated_cmov<T, index_t, size>(arr, q);
    } else {
      return templated_binary<T, index_t, size>(arr, q);
    }
  }
#else
  if constexpr (sizeof(T) == 1) {
    if constexpr (size >= 512) {
      return templated_cmov<T, index_t, size>(arr, q);
    } else {
      return linear_scan_cmov<T, index_t, size>(arr, q);
    }
  } else if constexpr (sizeof(T) <= 2) {
    if constexpr (size >= 1024) {
      return templated_cmov<T, index_t, size>(arr, q);
    } else {
      return linear_scan_cmov<T, index_t, size>(arr, q);
    }
  } else if constexpr (sizeof(T) <= 4) {
    if constexpr (size >= 128) {
      return templated_cmov<T, index_t, size>(arr, q);
    } else {
      return linear_scan_cmov<T, index_t, size>(arr, q);
    }
  } else {
    if constexpr (size >= 32) {
      return templated_cmov<T, index_t, size>(arr, q);
    } else {
      return templated_binary<T, index_t, size>(arr, q);
    }
  }
#endif
}

template <class arr_t, class index_t = unsigned short>
index_t search(const arr_t arr, typename arr_t::value_type q) {
  static_assert(std::numeric_limits<index_t>::max() >= arr.size());
  return search<typename arr_t::value_type, index_t, arr.size()>(arr.data(), q);
}

template <class T>
constexpr T max_val() {
  static_assert(std::numeric_limits<T>::is_specialized,
                "For non-numeric (non-trivial) types, max and a total order "
                "must be provided.");
  if constexpr (std::is_floating_point<T>::value) {
    return std::numeric_limits<T>::infinity();
  }
  return std::numeric_limits<T>::max();
}

template <uint64_t block_size>
constexpr uint16_t max_levels() {
  return 64 / __builtin_ctz(block_size);
}
}  // namespace bt::internal
