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
	VKSwapchain::VKSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) : Swapchain(createInfo)
	{
		VkSurfaceKHR surface = static_cast<VKSurface *>(pWindow->GetSurface())->GetHandle();

		auto presentQueueFamilyIndex = static_cast<VKDevice *>(createInfo.pDevice)->GetPresentQueueFamilyIndex(pWindow);

		if (!presentQueueFamilyIndex.has_value())
			THROW("current device do not support present to surface");

		mPresentQueueFamilyIndex = presentQueueFamilyIndex.value();

		//set format
		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		VkPhysicalDevice physicalDevice = static_cast<VKDevice *>(createInfo.pDevice)->GetPhysicalDevice();

		VK::querySurfaceFormats(physicalDevice, surface, surfaceFormats);

		mSurfaceFormat = surfaceFormats[0]; //default rgba8unorm

		VkFormat format = Map(createInfo.format);
		VkColorSpaceKHR colorSpace = Map(createInfo.colorSpace);
		//change to srgba8
		for (auto &&e : surfaceFormats)
		{
			if (e.format == format && e.colorSpace == colorSpace)
			{
				mSurfaceFormat = e;
				break;
			}
		}

		std::vector<VkPresentModeKHR> presentModes;
		VK::querySurfacePresentModes(physicalDevice, surface, presentModes);

		auto dstmode = Map(createInfo.presentMode);
		for (auto &&e : presentModes)
		{
			if (dstmode == e)
			{
				mPresentMode = e;
				break;
			}
		}

		VkSwapchainCreateInfoKHR swapchainInfo{
			VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			NULL,
			0,
			surface,
			createInfo.minImageCount,
			mSurfaceFormat.format,
			mSurfaceFormat.colorSpace,
			{createInfo.imageExtent.width,
			 createInfo.imageExtent.height},
			1,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			mPresentMode,
			VK_TRUE,
			VK_NULL_HANDLE};

		if (vkCreateSwapchainKHR(
				static_cast<VKDevice *>(createInfo.pDevice)->GetHandle(),
				&swapchainInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create swapchain");
	}
} // namespace Shit
