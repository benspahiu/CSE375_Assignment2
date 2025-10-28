#pragma once
// #include <functional>
// #include <cstdint>
// #include <cstddef>
#include <concepts>
// #include <type_traits>

namespace myhash {

template <typename T>
concept Hashable = requires(T a) {
    { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept HashableAndEquatable = Hashable<T> && std::equality_comparable<T>;

inline uint64_t mix64(uint64_t x) noexcept {
    // A strong bit-mixing function
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33;
    x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return x;
}

template <Hashable T>
struct StdHash1 {
    size_t operator()(const T& key) const noexcept {
        uint64_t h = std::hash<T>{}(key);
        return static_cast<size_t>(mix64(h ^ 0x9e3779b97f4a7c15ULL)); // seed1
    }
};

template <Hashable T>
struct StdHash2 {
    size_t operator()(const T& key) const noexcept {
        uint64_t h = std::hash<T>{}(key);
        return static_cast<size_t>(mix64(h ^ 0xbf58476d1ce4e5b9ULL)); // seed2
    }
};

} // namespace myhash