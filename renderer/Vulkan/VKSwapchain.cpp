/**
 * @file VKSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKSwapchain.h"
namespace Shit
{
	VKSwapchain::VKSwapchain(
		VkDevice device,
		const SwapchainCreateInfo &createInfo,
		VkSurfaceKHR surface,
		VkSurfaceFormatKHR surfaceFormat,
		VkPresentModeKHR presentMode,
		QueueFamilyIndex presentQueueFamilyIndex)
		: Swapchain(createInfo), mDevice(device)
	{
		mPresentQueueFamilyIndex = presentQueueFamilyIndex;

		VkSwapchainCreateInfoKHR swapchainInfo{
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			NULL,
			0,
			surface,
			createInfo.minImageCount,
			surfaceFormat.format,
			surfaceFormat.colorSpace,
			{createInfo.imageExtent.width,
			 createInfo.imageExtent.height},
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			presentMode,
			VK_TRUE,
			VK_NULL_HANDLE};

		if (vkCreateSwapchainKHR(mDevice, &swapchainInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create swapchain");

		uint32_t swapchainImageCount;
		vkGetSwapchainImagesKHR(mDevice, mHandle, &swapchainImageCount, nullptr);
		LOG(swapchainImageCount);
		std::vector<VkImage> swapchainImages;
		swapchainImages.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(mDevice, mHandle, &swapchainImageCount, swapchainImages.data());
		for (auto e : swapchainImages)
		{
			mImages.emplace_back(std::make_unique<VKImage>(mDevice, e, true));
		}
	}
} // namespace Shit
