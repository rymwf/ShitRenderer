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
		VkImage mHandle;
		VkDeviceMemory mMemory;
		Device *mpDevice;

		void GenerateMipmaps(Filter filter);

	public:
		VKImage(Device *pDevice, const ImageCreateInfo &createInfo, const void *pData);
		VKImage(Device *pDevice, VkImage image, bool isSwapchainimage) : mpDevice(pDevice), mHandle(image)
		{
			mIsSwapchainImage = isSwapchainimage;
		}

		~VKImage() override;

		constexpr VkImage GetHandle() const
		{
			return mHandle;
		}
		void UpdateSubData(uint32_t mipLevel, const Rect3D rect, const void *pData) override;
	};

	class VKImageView final : public ImageView
	{
		VkDevice mDevice;
		VkImageView mHandle;

	public:
		VKImageView(VkDevice device, const ImageViewCreateInfo &createInfo);
		~VKImageView() override
		{
			vkDestroyImageView(mDevice, mHandle, nullptr);
		}
		constexpr VkImageView GetHandle() const
		{
			return mHandle;
		}
	};

} // namespace Shit
