/**
 * @file VKImage.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKImage.hpp"
#include "VKBuffer.hpp"
#include "VKDevice.hpp"
#include "VKCommandBuffer.hpp"
namespace Shit
{
	VKImage::~VKImage()
	{
		if (!mIsSwapchainImage)
		{
			vkDestroyImage(static_cast<VKDevice *>(mpDevice)->GetHandle(), mHandle, nullptr);
			vkFreeMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory, nullptr);
		}
	}
	VKImage::VKImage(Device *pDevice, const ImageCreateInfo &createInfo, const void *pData)
		: Image(createInfo), mpDevice(pDevice)
	{
		auto device = static_cast<VKDevice *>(pDevice)->GetHandle();
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

		if (vkCreateImage(device, &imageCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create image");

		VkMemoryRequirements imageMemoryRequireMents;
		vkGetImageMemoryRequirements(device, mHandle, &imageMemoryRequireMents);
		LOG(imageMemoryRequireMents.size);
		LOG(imageMemoryRequireMents.alignment);
		LOG(imageMemoryRequireMents.memoryTypeBits);

		auto memoryTypeIndex = VK::findMemoryTypeIndex(static_cast<VKDevice *>(pDevice)->GetPhysicalDevice(), imageMemoryRequireMents.memoryTypeBits,
													   Map(createInfo.memoryPropertyFlags));

		mMemory = VK::allocateMemory(device, imageMemoryRequireMents.size, memoryTypeIndex);

		vkBindImageMemory(device, mHandle, mMemory, 0);

		if (pData)
		{
			UpdateSubData(0, {{}, mCreateInfo.extent}, pData);
			if (mCreateInfo.generateMipmap)
			{
				GenerateMipmaps(mCreateInfo.mipmapFilter);
			}
		}
	}
	void VKImage::UpdateSubData(uint32_t mipLevel, const Rect3D rect, const void *pData)
	{
		uint64_t size = rect.extent.width * rect.extent.height * rect.extent.depth * GetFormatSize(mCreateInfo.format);

		BufferCreateInfo stagingBufferCreateInfo{
			{},
			size,
			BufferUsageFlagBits::TRANSFER_SRC_BIT,
			MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
		};

		VKBuffer stagingbuffer{static_cast<VKDevice *>(mpDevice)->GetHandle(), static_cast<VKDevice *>(mpDevice)->GetPhysicalDevice(), stagingBufferCreateInfo};
		void *data;
		stagingbuffer.MapBuffer(0, size, &data);
		memcpy(data, pData, static_cast<size_t>(size));
		stagingbuffer.UnMapBuffer();

		static_cast<VKDevice *>(mpDevice)->ExecuteOneTimeCommands([&](CommandBuffer *pCommandBuffer) {
			VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(mCreateInfo.format);

			//1. transfer layout from undefined to transder destination
			VkImageMemoryBarrier barrier{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				mHandle,
				{aspectFlags,
				 mipLevel,
				 mCreateInfo.mipLevels,
				 static_cast<uint32_t>(rect.offset.z),
				 rect.extent.depth}};
			vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
								 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //src stage mask
								 VK_PIPELINE_STAGE_TRANSFER_BIT,	//dst stage mask
								 0,									//dependency flags
								 0, nullptr,						//memory barriers
								 0, nullptr,						//buffer memory barriers
								 1, &barrier);						//image memory barriers
			//2. copy image
			BufferImageCopy bufferImageCopy{
				0,																	 //buffer offset
				0,																	 //buffer row length
				0,																	 //buffer image height
				{mipLevel, static_cast<uint32_t>(rect.offset.z), rect.extent.depth}, //image subresource
				rect.offset,														 //image offset
				rect.extent															 //image extent
			};
			pCommandBuffer->CopyBufferToImage({&stagingbuffer,
											   this,
											   1,
											   &bufferImageCopy});
			if (!mCreateInfo.generateMipmap)
			{
				////3. transfer layout from trander destiation to shader reading
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
									 0,
									 0, nullptr,
									 0, nullptr,
									 1, &barrier);
			}
		});
	}
	void VKImage::GenerateMipmaps(Filter filter)
	{
		auto mipExtent = mCreateInfo.extent;
		auto mipLevels = (std::min)(
			static_cast<uint32_t>(std::floor(std::log2((std::max)((std::max)(mipExtent.width, mipExtent.height), mipExtent.depth))) + 1),
			mCreateInfo.mipLevels);
		auto imageAspect = GetImageAspectFromFormat(mCreateInfo.format);

		VkImageMemoryBarrier barrier{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			mHandle,
			{imageAspect,
			 0,
			 1,
			 0,
			 mCreateInfo.arrayLayers}};

		VkImageBlit blitRegion{
			//src
			{
				imageAspect,
				0, //miplevel
				0, //base array layer
				mCreateInfo.arrayLayers  //layer count
			},
			{{}, {}}, //offset
			//dst
			{imageAspect,
			 1,
			 0,
			 mCreateInfo.arrayLayers},
			{{}, {}} //offset
		};

		static_cast<VKDevice *>(mpDevice)->ExecuteOneTimeCommands([&](CommandBuffer *pCommandBuffer) {
			for (uint32_t i = 1; i < mipLevels; ++i)
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.subresourceRange.baseMipLevel = i - 1;

				vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 0,
									 0, nullptr,
									 0, nullptr,
									 1, &barrier);

				blitRegion.srcSubresource.mipLevel = i - 1;
				blitRegion.srcOffsets[1] = {static_cast<int32_t>(mipExtent.width), static_cast<int32_t>(mipExtent.height), static_cast<int32_t>(mipExtent.depth)};

				mipExtent.width = (std::max)(mipExtent.width >> 1, 1u);
				mipExtent.height = (std::max)(mipExtent.height >> 1, 1u);
				mipExtent.depth = (std::max)(mipExtent.depth >> 1, 1u);

				blitRegion.dstSubresource.mipLevel = i;
				blitRegion.dstOffsets[1] = {static_cast<int32_t>(mipExtent.width), static_cast<int32_t>(mipExtent.height), static_cast<int32_t>(mipExtent.depth)};
				vkCmdBlitImage(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
							   mHandle,								 //srcImage
							   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //src image layout
							   mHandle,								 //dstimage
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //dst image layout
							   1,									 //region count
							   &blitRegion,							 //regions
							   Map(filter));
			}
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.subresourceRange.baseMipLevel = mipLevels - 1;

			vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);

			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount= mipLevels - 1;

			vkCmdPipelineBarrier(static_cast<VKCommandBuffer *>(pCommandBuffer)->GetHandle(),
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);
		});
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
