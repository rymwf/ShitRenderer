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
#include "VKSurface.h"
#include "VKImage.h"

namespace Shit
{

	class VKSwapchain final : public Swapchain
	{
		VkSwapchainKHR mHandle;
		VkPresentModeKHR mPresentMode{};
		VkSurfaceFormatKHR mSurfaceFormat;
		VkDevice mDevice;

	public:
		VKSwapchain(
			VkDevice device,
			const SwapchainCreateInfo &createInfo,
			VkSurfaceKHR surface,
			VkSurfaceFormatKHR surfaceFormat,
			VkPresentModeKHR presentMode,
			QueueFamilyIndex presentQueueFamilyIndex
			);

		constexpr VkPresentModeKHR GetPresentMode() const
		{
			return mPresentMode;
		}
		~VKSwapchain() override
		{
			vkDestroySwapchainKHR(mDevice, mHandle, nullptr);
		}
	};

}
