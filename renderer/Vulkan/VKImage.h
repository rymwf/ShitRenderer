/**
 * @file VKImage.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitImage.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKImage final : public Image
	{
		VkDevice mDevice;
		VkImage mHandle;
		VkDeviceMemory mMemory;
		VkPhysicalDevice mPhysicalDevice;

	public:
		VKImage(VkDevice device, VkPhysicalDevice physicalDevice, const ImageCreateInfo &createInfo);
		VKImage(VkDevice device, VkImage image, bool isSwapchainimage) : mDevice(device), mHandle(image)
		{
			mIsSwapchainImage = isSwapchainimage;
		}

		~VKImage() override
		{
			if (!mIsSwapchainImage)
				vkDestroyImage(mDevice, mHandle, nullptr);
		}
		constexpr VkImage GetHandle() const
		{
			return mHandle;
		}
		void UpdateSubData();
	};

	class VKImageView final : public ImageView
	{
		VkDevice mDevice;
		VkImageView mHandle;

	public:
		VKImageView(VkDevice device, const ImageViewCreateInfo &createInfo);
		~VKImageView() override
		{
			vkDestroyImageView(mDevice,mHandle,nullptr);
		}
		constexpr VkImageView GetHandle() const
		{
			return mHandle;
		}
	};

} // namespace Shit
