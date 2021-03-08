/**
 * @file VKImage.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKImage.h"
namespace Shit
{
	VKImage::VKImage(VkDevice device, VkPhysicalDevice physicalDevice, const ImageCreateInfo &createInfo)
		: Image(createInfo), mDevice(device), mPhysicalDevice(physicalDevice)
	{
		VkImageCreateInfo imageCreateInfo{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			Map(createInfo.flags),
			Map(createInfo.imageType),
			Map(createInfo.format),
			VkExtent3D{createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth},
			createInfo.mipLevels,
			createInfo.arrayLayers,
			static_cast<VkSampleCountFlagBits>(createInfo.samples),
			Map(createInfo.tiling),
			Map(createInfo.usageFlags) | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			VK_IMAGE_LAYOUT_UNDEFINED};

		if (vkCreateImage(mDevice, &imageCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create image");

		VkMemoryRequirements imageMemoryRequireMents;
		vkGetImageMemoryRequirements(mDevice, mHandle, &imageMemoryRequireMents);
		LOG(imageMemoryRequireMents.size);
		LOG(imageMemoryRequireMents.alignment);
		LOG(imageMemoryRequireMents.memoryTypeBits);

		auto memoryTypeIndex = VK::findMemoryTypeIndex(mPhysicalDevice, imageMemoryRequireMents.memoryTypeBits,
													   Map(createInfo.memoryPropertyFlags));

		mMemory = VK::allocateMemory(mDevice, imageMemoryRequireMents.size, memoryTypeIndex);

		vkBindImageMemory(mDevice, mHandle, mMemory, 0);
	}
	void VKImage::UpdateSubData()
	{
	}

	//=======================================================================================

	VKImageView::VKImageView(VkDevice device, const ImageViewCreateInfo &createInfo)
		: ImageView(createInfo), mDevice(device)
	{
		VkImageViewCreateInfo imageViewCreateInfo{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			static_cast<VKImage *>(createInfo.pImage)->GetHandle(),
			Map(createInfo.viewType),
			Map(createInfo.format),
			{
				Map(createInfo.components.r),
				Map(createInfo.components.g),
				Map(createInfo.components.b),
				Map(createInfo.components.a),
			},
			{
				GetImageAspectFromFormat(createInfo.format),
				createInfo.subresourceRange.baseMipLevel,
				createInfo.subresourceRange.levelCount,
				createInfo.subresourceRange.baseArrayLayer,
				createInfo.subresourceRange.layerCount,
			}};
		if (vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create image view");
	}
} // namespace Shit
