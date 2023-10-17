/**
 * @file ShitUtility.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <forward_list>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#include "ShitPredefines.hpp"

#if 0  //__cplusplus > 201703L
// C++20 code
#include <execution>
#endif

namespace Shit {
template <class T_, class Container_>
void RemoveSmartPtrFromContainer(Container_ &container, const T_ *entry) {
    if (entry) {
#if ST_HAS_CXX20
        // nodiscard warnning
        auto [first, last] = std::remove(container, entry, [](auto &&e) { return e.get(); });
        container.erase(first, last);
#else
        auto it = std::remove_if(container.begin(), container.end(), [entry](auto &&e) { return e.get() == entry; });
        container.erase(it, container.end());
#endif
    }
}

#if __cplusplus > 201703L
template <typename T, typename... U>
concept IsAnyOf = (std::same_as<T, U> || ...);

// Declaration of the concept "Hashable", which is satisfied by any type 'T'
// such that for values 'a' of type 'T', the expression std::hash<T>{}(a)
// compiles and its result is convertible to std::size_t

template <typename T>
concept Hashable = requires(T a) {
                       { std::hash<T>{}(a) } -> std::convertible_to<std::size_t>;
                   };
#endif

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename charT, class... Args>
constexpr void myprint(std::basic_ostream<charT> &os, Args &&...args) noexcept {
    ((os << std::forward<Args>(args) << " "), ...);
}
// template <typename T, typename...Ts>
// constexpr void print(T&& first, Ts&&... rest) noexcept
// {
//    if constexpr (sizeof...(Ts) == 0)
//    {
//       std::cout << first;               // for only 1-arguments
//    }
//    else
//    {
//       std::cout << first << " ";        // print the 1 argument
//       print(std::forward<Ts>(rest)...); // pass the rest further
//    }
// }

template <typename F, typename... Args>
uint64_t timeCostMs(const F &func, Args... args) {
    static std::chrono::high_resolution_clock::time_point t;
    t = std::chrono::high_resolution_clock::now();
    func(&args...);
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t).count();
}

std::vector<std::string_view> split(std::string_view str, std::string_view delim);
}  // namespace Shit
