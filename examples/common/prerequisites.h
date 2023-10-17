#pragma once
#include "exampleconfig.h"
#include "renderer/ShitRenderSystem.hpp"
#include "singleton.h"
#include "timer.h"

template <class... Args>
constexpr void myprint(std::ostream &os, Args &&...args) noexcept {
    ((os << std::forward<Args>(args) << " "), ...);
}

#ifdef NDEBUG
#define LOG(...)
#define LOG_VAR(str)
#else
#define LOG(...)                                          \
    {                                                     \
        std::cout << __FILE__ << " " << __LINE__ << ": "; \
        myprint(std::cout, __VA_ARGS__);                  \
        std::cout << std::endl;                           \
    }
#define LOG_VAR(...)                                                             \
    {                                                                            \
        std::cout << __FILE__ << " " << __LINE__ << " " << #__VA_ARGS__ << ": "; \
        myprint(std::cout, __VA_ARGS__);                                         \
        std::cout << std::endl;                                                  \
    }
// std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str <<
// std::endl;
#endif

#define THROW(...)                            \
    {                                         \
        std::stringstream ss;                 \
        myprint(ss, __FILE__, __LINE__, ":"); \
        myprint(ss, __VA_ARGS__);             \
        throw std::runtime_error(ss.str());   \
    }
#define CHECK_VK_RESULT(x)                                            \
    {                                                                 \
        auto res = x;                                                 \
        if (res != VK_SUCCESS) THROW(#x " failed, error code:", res); \
    }

class GameObject;
class Component;

constexpr uint32_t getTopologyFaceStep(Shit::PrimitiveTopology topology) {
    switch (topology) {
        case Shit::PrimitiveTopology::LINE_LIST:
        case Shit::PrimitiveTopology::LINE_LIST_WITH_ADJACENCY:
            return 2;
        case Shit::PrimitiveTopology::TRIANGLE_LIST:
        case Shit::PrimitiveTopology::TRIANGLE_LIST_WITH_ADJACENCY:
            return 3;
        default:
            return 1;
    }
}

#if defined(_MSC_VER)
#define FUNCTIONAL_HASH_ROTL32(x, r) _rotl(x, r)
#else
#define FUNCTIONAL_HASH_ROTL32(x, r) (x << r) | (x >> (32 - r))
#endif

// from boost
template <typename T>
void hashCombineImpl(T &seed, T value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline void hashCombineImpl(uint32_t &h1, uint32_t k1) {
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    k1 *= c1;
    k1 = FUNCTIONAL_HASH_ROTL32(k1, 15);
    k1 *= c2;

    h1 ^= k1;
    h1 = FUNCTIONAL_HASH_ROTL32(h1, 13);
    h1 = h1 * 5 + 0xe6546b64;
}

constexpr void hashCombineImpl(uint64_t &h, uint64_t k) {
    const uint64_t m = UINT64_C(0xc6a4a7935bd1e995);
    const int r = 47;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;

    // Completely arbitrary number, to prevent 0's
    // from hashing to 0.
    h += 0xe6546b64;
}

template <typename T>
void hashCombine(std::size_t &seed, const T &v) {
    hashCombineImpl(seed, std::hash<T>{}(v));
}