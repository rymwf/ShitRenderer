/**
 * @file ShitNoncopyable.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
namespace Shit
{

	class NonCopyable
	{
	public:
		NonCopyable() = default;
		NonCopyable(const NonCopyable &other) = delete;
		NonCopyable &operator=(const NonCopyable &other) = delete;
	};
} // namespace Shit
