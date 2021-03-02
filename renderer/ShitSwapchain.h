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

	public:
		Swapchain(const SwapchainCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Swapchain() {}
	};
} // namespace Shit
