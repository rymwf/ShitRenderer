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

#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <optional>
#include <stack>
#include <utility>
#include <array>
#include <unordered_map>
#include <functional>
#include <type_traits>
#include <variant>
#include <algorithm>
#include <execution>
#include <sstream>
#include <list>
#include <forward_list>

namespace Shit
{
	template <class T_, class Container_>
	void RemoveSmartPtrFromContainer(Container_ &container, const T_ *entry)
	{
		if (entry)
		{
#if __cplusplus >= 201703L
			auto &&it = std::find_if(std::execution::par_unseq, container.begin(), container.end(), [entry](auto &&e) {
				return e.get() == entry;
			});
#else
			auto &&it = std::find_if(container.begin(), container.end(), [entry](auto &&e) {
				return e.get() == entry;
			});
#endif
			if (it != container.end())
				container.erase(it);
		}
	}

	template <typename T, typename... U>
	concept IsAnyOf = (std::same_as<T, U> || ...);

	// Declaration of the concept "Hashable", which is satisfied by any type 'T'
	// such that for values 'a' of type 'T', the expression std::hash<T>{}(a)
	// compiles and its result is convertible to std::size_t
	template <typename T>
	concept Hashable = requires(T a)
	{
		{
			std::hash<T>{}(a)
		}
		->std::convertible_to<std::size_t>;
	};

	template <class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};
	// explicit deduction guide (not needed as of C++20)
	template <class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

} // namespace Shit
