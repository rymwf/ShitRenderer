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

namespace Shit
{

	class VKSwapchain final : public Swapchain
	{
		VkSwapchainKHR mHandle;
		VkPresentModeKHR mPresentMode{};
		VkSurfaceFormatKHR mSurfaceFormat;

		VKDevice* mpDevice;
	public:
		VKSwapchain(Device *pDevice, ShitWindow *pWindow, const SwapchainCreateInfo &createInfo);

		constexpr VkSurfaceFormatKHR GetSurfaceFormat() const
		{
			return mSurfaceFormat;
		}
		constexpr VkSwapchainKHR GetHandle() const
		{
			return mHandle;
		}
		constexpr VkPresentModeKHR GetPresentMode() const
		{
			return mPresentMode;
		}
		~VKSwapchain() override;

		Result GetNextImage(const GetNextImageInfo &info, uint32_t& index) override;
	};

}
