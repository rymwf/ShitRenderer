/**
 * @file VKSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <renderer/ShitWindow.hpp>
#include "VKSwapchain.hpp"
#include "VKSemaphore.hpp"
#include "VKFence.hpp"
#include "VKDevice.hpp"
#include "VKSurface.hpp"
#include "VKImage.hpp"

namespace Shit
{
	VKSwapchain::~VKSwapchain()
	{
		vkDestroySwapchainKHR(mpDevice->GetHandle(), mHandle, nullptr);
	}
	VKSwapchain::VKSwapchain(Device *pDevice, ShitWindow *pWindow, const SwapchainCreateInfo &createInfo)
		: Swapchain(createInfo), mpDevice(static_cast<VKDevice *>(pDevice))
	{
		VkSurfaceKHR surface = static_cast<const VKSurface *>(pWindow->GetSurfacePtr())->GetHandle();

		auto presentQueueFamilyIndex = pDevice->GetPresentQueueFamilyIndex(pWindow);

		if (!presentQueueFamilyIndex.has_value())
			THROW("current device do not support present to surface");
		mPresentQueueFamilyIndex = presentQueueFamilyIndex.value();

		//set format
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		VK::querySurfaceFormats(mpDevice->GetPhysicalDevice(), surface, surfaceFormats);

		VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];

		VkFormat format = Map(createInfo.format);
		VkColorSpaceKHR colorSpace = Map(createInfo.colorSpace);
		//change to srgba8
		for (auto &&e : surfaceFormats)
		{
			if (e.format == format && e.colorSpace == colorSpace)
			{
				surfaceFormat = e;
				break;
			}
			LOG("swapchain format do not support,use format: " + std::to_string(surfaceFormat.format));
		}
		LOG_VAR(surfaceFormat.format);
		LOG_VAR(surfaceFormat.colorSpace);

		std::vector<VkPresentModeKHR> presentModes;
		VK::querySurfacePresentModes(mpDevice->GetPhysicalDevice(), surface, presentModes);
		VkPresentModeKHR presentMode{VK_PRESENT_MODE_IMMEDIATE_KHR};
		auto dstmode = Map(createInfo.presentMode);
		for (auto &&e : presentModes)
		{
			if (dstmode == e)
			{
				presentMode = e;
				break;
			}
		}

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
			Map(createInfo.imageUsage),
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
			VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			presentMode,
			VK_TRUE,
			VK_NULL_HANDLE};

		CHECK_VK_RESULT(vkCreateSwapchainKHR(mpDevice->GetHandle(), &swapchainInfo, nullptr, &mHandle));

		uint32_t swapchainImageCount;
		vkGetSwapchainImagesKHR(mpDevice->GetHandle(), mHandle, &swapchainImageCount, nullptr);
		LOG_VAR(swapchainImageCount);
		std::vector<VkImage> swapchainImages;
		swapchainImages.resize(swapchainImageCount);
		vkGetSwapchainImagesKHR(mpDevice->GetHandle(), mHandle, &swapchainImageCount, swapchainImages.data());

		ImageCreateInfo imageCreateInfo{
			{},
			ImageType::TYPE_2D,
			createInfo.format,
			{createInfo.imageExtent.width,
			 createInfo.imageExtent.height, 1},
			1,
			1,
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			createInfo.imageUsage,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			{},
			ImageLayout::PRESENT_SRC};
		for (auto e : swapchainImages)
		{
			mImages.emplace_back(std::make_unique<VKImage>(mpDevice, imageCreateInfo, e, true));
		}
	}
	Result VKSwapchain::GetNextImage(const GetNextImageInfo &info, uint32_t &index)
	{
		auto res = vkAcquireNextImageKHR(mpDevice->GetHandle(), mHandle, info.timeout,
										 info.pSemaphore ? static_cast<VKSemaphore *>(info.pSemaphore)->GetHandle() : VK_NULL_HANDLE,
										 info.pFence ? static_cast<VKFence *>(info.pFence)->GetHandle() : VK_NULL_HANDLE,
										 &index);
		switch (res)
		{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			return Result::SUCCESS;
		case VK_TIMEOUT:
			return Result::TIMEOUT;
		case VK_NOT_READY:
			return Result::NOT_READY;
		case VK_ERROR_OUT_OF_DATE_KHR:
			return Result::SHIT_ERROR_OUT_OF_DATE;
		//case VK_ERROR_OUT_OF_HOST_MEMORY:
		//case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		//case VK_ERROR_DEVICE_LOST:
		//case VK_ERROR_SURFACE_LOST_KHR:
		//case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
		default:
			THROW("failed to aquire next image, error code: " + std::to_string(res));
		}
	}
} // namespace Shit
