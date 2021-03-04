/**
 * @file VKSwapchain.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSwapchain.h>
#include "VKPrerequisites.h"
#include "VKDevice.h"
#include "VKSurface.h"

namespace Shit
{

	class VKSwapchain final : public Swapchain
	{
		VkSwapchainKHR mHandle;
		QueueFamilyIndex mPresentQueueFamilyIndex;
		VkPresentModeKHR mPresentMode{};
		VkSurfaceFormatKHR mSurfaceFormat;

	public:
		VKSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow);

		QueueFamilyIndex GetPresentQueueFamilyIndex() const
		{
			return mPresentQueueFamilyIndex;
		}
		VkPresentModeKHR GetPresentMode() const
		{
			return mPresentMode;
		}
		~VKSwapchain() override
		{
			vkDestroySwapchainKHR(static_cast<VKDevice*>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);
		}
	};

}
