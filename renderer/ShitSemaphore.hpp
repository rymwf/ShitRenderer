/**
 * @file ShitSemaphore.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit
{
	class Semaphore
	{
	protected:
		SemaphoreCreateInfo mCreateInfo;
		Semaphore(const SemaphoreCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~Semaphore() {}
		constexpr const SemaphoreCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
