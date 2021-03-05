/**
 * @file ShitSwapchain.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"

namespace Shit
{
	class Swapchain
	{
	protected:
		SwapchainCreateInfo mCreateInfo;
		QueueFamilyIndex mPresentQueueFamilyIndex{0, INT_MAX};

	public:
		Swapchain(const SwapchainCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		constexpr QueueFamilyIndex GetPresentQueueFamilyIndex() const
		{
			return mPresentQueueFamilyIndex;
		}
		virtual ~Swapchain() {}
	};
} // namespace Shit
