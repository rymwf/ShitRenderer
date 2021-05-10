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
		if (mCreateInfo.mipLevels == 0)
			mCreateInfo.mipLevels = static_cast<uint32_t>(
				std::floor(
					std::log2((std::max)(
						(std::max)(mCreateInfo.extent.width, mCreateInfo.extent.height),
						mCreateInfo.extent.depth))) +
				1);
		auto device = static_cast<VKDevice *>(pDevice)->GetHandle();

#ifndef NDEBUG
		VkImageFormatProperties imageFormatProperties;
		vkGetPhysicalDeviceImageFormatProperties(
			static_cast<VKDevice *>(mpDevice)->GetPhysicalDevice(),
			Map(mCreateInfo.format),
			Map(mCreateInfo.imageType),
			Map(mCreateInfo.tiling),
			Map(mCreateInfo.usageFlags),
			Map(mCreateInfo.flags),
			&imageFormatProperties);
#endif

		ImageLayout tempLayout = mCreateInfo.initialLayout == ImageLayout::PREINITIALIZED ? ImageLayout::PREINITIALIZED : ImageLayout::UNDEFINED;
		mCreateInfo.usageFlags |= ImageUsageFlagBits::TRANSFER_SRC_BIT | ImageUsageFlagBits::TRANSFER_DST_BIT;

		VkImageCreateInfo imageCreateInfo{
			VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			nullptr,
			Map(mCreateInfo.flags),
			Map(mCreateInfo.imageType),
			Map(mCreateInfo.format),
			VkExtent3D{mCreateInfo.extent.width, mCreateInfo.extent.height, mCreateInfo.extent.depth},
			mCreateInfo.mipLevels,
			mCreateInfo.arrayLayers,
			static_cast<VkSampleCountFlagBits>(mCreateInfo.samples),
			Map(mCreateInfo.tiling),
			Map(mCreateInfo.usageFlags),
			VK_SHARING_MODE_EXCLUSIVE,
			0,
			nullptr,
			Map(tempLayout)};

		if (vkCreateImage(device, &imageCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create image");

		VkMemoryRequirements imageMemoryRequireMents;
		vkGetImageMemoryRequirements(device, mHandle, &imageMemoryRequireMents);

		mMemorySize = imageMemoryRequireMents.size;

		LOG_VAR(imageMemoryRequireMents.size);
		LOG_VAR(imageMemoryRequireMents.alignment);
		LOG_VAR(imageMemoryRequireMents.memoryTypeBits);

		auto memoryTypeIndex = VK::findMemoryTypeIndex(static_cast<VKDevice *>(pDevice)->GetPhysicalDevice(), imageMemoryRequireMents.memoryTypeBits,
													   Map(mCreateInfo.memoryPropertyFlags));

		mMemory = VK::allocateMemory(device, imageMemoryRequireMents.size, memoryTypeIndex);

		vkBindImageMemory(device, mHandle, mMemory, 0);

		if (pData)
		{
			UpdateSubData(0,
						  tempLayout,
						  mCreateInfo.initialLayout,
						  {{}, mCreateInfo.extent}, pData);
		}
		else if (mCreateInfo.initialLayout != ImageLayout::UNDEFINED && mCreateInfo.initialLayout != ImageLayout::PREINITIALIZED)
		{
			auto queueFamilyIndex = mpDevice->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {});
			VkQueue queue;
			vkGetDeviceQueue(device, queueFamilyIndex->index, 0, &queue);

			VkCommandPool commandPool;
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				nullptr,
				VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
				queueFamilyIndex->index};
			CHECK_VK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool))

			VkCommandBuffer commandBuffer;
			VkCommandBufferAllocateInfo commandBufferAllocateInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				nullptr,
				commandPool,
				VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				1};
			CHECK_VK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer))

			VkCommandBufferBeginInfo beginInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
			};

			vkBeginCommandBuffer(commandBuffer, &beginInfo);

			VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(mCreateInfo.format);
			//transfer layout from to initial layout
			VkImageMemoryBarrier barrier{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_SHADER_READ_BIT,
				Map(tempLayout),
				Map(mCreateInfo.initialLayout),
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				mHandle,
				{aspectFlags, 0, mCreateInfo.mipLevels, 0, mCreateInfo.arrayLayers}};

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
								 0,			   //dependency flags
								 0, nullptr,   //memory barriers
								 0, nullptr,   //buffer memory barriers
								 1, &barrier); //image memory barriers

			vkEndCommandBuffer(commandBuffer);

			VkSubmitInfo submitInfo{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,
				nullptr,
				0,
				nullptr,
				nullptr,
				1,
				&commandBuffer,
				0,
				nullptr};
			CHECK_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE))
			vkQueueWaitIdle(queue);
			vkDestroyCommandPool(device, commandPool, nullptr);
		}
	}
	void VKImage::UpdateSubData(uint32_t mipLevel, ImageLayout initialLayout, ImageLayout finalLayout, const Rect3D &rect, const void *pData)
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
		stagingbuffer.MapMemory(0, size, &data);
		memcpy(data, pData, static_cast<size_t>(size));
		stagingbuffer.UnMapMemory();

		//=================================================================================
		VkDevice device = static_cast<VKDevice *>(mpDevice)->GetHandle();

		auto queueFamilyIndex = mpDevice->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {});
		VkQueue queue;
		vkGetDeviceQueue(device, queueFamilyIndex->index, 0, &queue);

		VkCommandPool commandPool;
		VkCommandPoolCreateInfo commandPoolCreateInfo{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			queueFamilyIndex->index};
		CHECK_VK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool))

		VkCommandBuffer commandBuffer;
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1};
		CHECK_VK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer))

		VkCommandBufferBeginInfo beginInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(mCreateInfo.format);

		//1. transfer layout from undefined to transder destination
		VkImageMemoryBarrier barrier{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			Map(initialLayout),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			mHandle,
			{aspectFlags,
			 mipLevel,
			 mCreateInfo.mipLevels,
			 static_cast<uint32_t>(rect.offset.z),
			 (std::max)(rect.extent.depth - static_cast<uint32_t>(rect.offset.z), 1u)}};
		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, //src stage mask
							 VK_PIPELINE_STAGE_TRANSFER_BIT,	//dst stage mask
							 0,									//dependency flags
							 0, nullptr,						//memory barriers
							 0, nullptr,						//buffer memory barriers
							 1, &barrier);						//image memory barriers
		//2. copy image
		VkBufferImageCopy bufferImageCopy{
			0,																				  //buffer offset
			0,																				  //buffer row length
			0,																				  //buffer image height
			{aspectFlags, mipLevel, static_cast<uint32_t>(rect.offset.z), rect.extent.depth}, //image subresource
			{rect.offset.x, rect.offset.y, rect.offset.z},									  //image offset
			{rect.extent.width, rect.extent.height, rect.extent.depth}						  //image extent
		};
		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingbuffer.GetHandle(),
			mHandle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&bufferImageCopy);
		////3. transfer layout
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = Map(finalLayout);

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
							 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&commandBuffer,
			0,
			nullptr};
		CHECK_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE))
		vkQueueWaitIdle(queue);
		vkDestroyCommandPool(device, commandPool, nullptr);
	}
	void VKImage::GenerateMipmaps(Filter filter, ImageLayout initialLayout, ImageLayout finalLayout)
	{
		auto mipExtent = mCreateInfo.extent;
		auto mipLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)((std::max)(mipExtent.width, mipExtent.height), mipExtent.depth))) + 1);
		mipLevels = (std::min)(mipLevels, mCreateInfo.mipLevels);
		auto imageAspect = GetImageAspectFromFormat(mCreateInfo.format);

		VkImageMemoryBarrier barrier{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			nullptr,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			Map(initialLayout),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_QUEUE_FAMILY_IGNORED,
			VK_QUEUE_FAMILY_IGNORED,
			mHandle,
			{imageAspect,
			 0,
			 mCreateInfo.mipLevels,
			 0,
			 mCreateInfo.arrayLayers}};

		VkImageBlit blitRegion{
			//src
			{
				imageAspect,
				0,						//miplevel
				0,						//base array layer
				mCreateInfo.arrayLayers //layer count
			},
			{{}, {}}, //offset
			//dst
			{imageAspect,
			 1,
			 0,
			 mCreateInfo.arrayLayers},
			{{}, {}} //offset
		};

		//=============================================
		VkDevice device = static_cast<VKDevice *>(mpDevice)->GetHandle();

		auto queueFamilyIndex = mpDevice->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {});
		VkQueue queue;
		vkGetDeviceQueue(device, queueFamilyIndex->index, 0, &queue);

		VkCommandPool commandPool;
		VkCommandPoolCreateInfo commandPoolCreateInfo{
			VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			nullptr,
			VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
			queueFamilyIndex->index};
		CHECK_VK_RESULT(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool))

		VkCommandBuffer commandBuffer;
		VkCommandBufferAllocateInfo commandBufferAllocateInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			commandPool,
			VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			1};
		CHECK_VK_RESULT(vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer))

		VkCommandBufferBeginInfo beginInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);
		barrier.subresourceRange.levelCount = 1;
		for (uint32_t i = 1; i < mipLevels; ++i)
		{
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange.baseMipLevel = i - 1;

			vkCmdPipelineBarrier(commandBuffer,
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
			vkCmdBlitImage(commandBuffer,
						   mHandle,								 //srcImage
						   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, //src image layout
						   mHandle,								 //dstimage
						   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, //dst image layout
						   1,									 //region count
						   &blitRegion,							 //regions
						   Map(filter));
		}

		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = Map(finalLayout);
		barrier.subresourceRange.baseMipLevel = mipLevels - 1;

		vkCmdPipelineBarrier(commandBuffer,
							 VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
							 0,
							 0, nullptr,
							 0, nullptr,
							 1, &barrier);

		if (mipLevels > 1)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = Map(finalLayout);
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mipLevels - 1;

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);
		}

		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{
			VK_STRUCTURE_TYPE_SUBMIT_INFO,
			nullptr,
			0,
			nullptr,
			nullptr,
			1,
			&commandBuffer,
			0,
			nullptr};
		CHECK_VK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE))
		vkQueueWaitIdle(queue);
		vkDestroyCommandPool(device, commandPool, nullptr);
	}
	void VKImage::MapMemory(uint64_t offset, uint64_t size, void **ppData)
	{
		vkMapMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory, offset, size, 0, ppData);
	}
	void VKImage::UnMapMemory()
	{
		vkUnmapMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory);
	}
	void VKImage::FlushMappedMemoryRange(uint64_t offset, uint64_t size)
	{
		auto range = VkMappedMemoryRange{
			VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
			nullptr,
			mMemory,
			offset,
			size};
		vkFlushMappedMemoryRanges(static_cast<VKDevice *>(mpDevice)->GetHandle(), 1, &range);
	}

	void VKImage::GetImageSubresourceLayout(const ImageSubresource &subresouce, SubresourceLayout &subresourceLayout)
	{
		VkImageSubresource a{
			GetImageAspectFromFormat(mCreateInfo.format),
			subresouce.mipLevel,
			subresouce.arrayLayer};
		VkSubresourceLayout ret;
		vkGetImageSubresourceLayout(
			static_cast<VKDevice *>(mpDevice)->GetHandle(),
			mHandle,
			&a,
			&ret);
		memcpy(&subresourceLayout, &ret, sizeof(ret));
	}
	//=======================================================================================

	VKImageView::VKImageView(VkDevice device, const ImageViewCreateInfo &createInfo)
		: ImageView(createInfo), mDevice(device)
	{
		VkImageViewCreateInfo imageViewCreateInfo{
			VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			nullptr,
			0,
			static_cast<VKImage *>(mCreateInfo.pImage)->GetHandle(),
			Map(mCreateInfo.viewType),
			Map(mCreateInfo.format),
			{
				Map(mCreateInfo.components.r),
				Map(mCreateInfo.components.g),
				Map(mCreateInfo.components.b),
				Map(mCreateInfo.components.a),
			},
			{
				GetImageAspectFromFormat(mCreateInfo.format),
				mCreateInfo.subresourceRange.baseMipLevel,
				mCreateInfo.subresourceRange.levelCount,
				mCreateInfo.subresourceRange.baseArrayLayer,
				mCreateInfo.subresourceRange.layerCount,
			}};
		if (vkCreateImageView(mDevice, &imageViewCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create image view");
	}
} // namespace Shit
