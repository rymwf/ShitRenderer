/**
 * @file VKCommandBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKCommandBuffer.h"
#include "VKBuffer.h"
#include "VKImage.h"
#include "VKFramebuffer.h"
#include "VKRenderPass.h"
#include "VKPipeline.h"
#include "VKBuffer.h"
namespace Shit
{
	VKCommandBuffer::VKCommandBuffer(VkDevice device, VkCommandPool commandPool, const CommandBufferCreateInfo &createInfo)
		: CommandBuffer(createInfo),
		  mDevice(device),
		  mCommandPool(commandPool)
	{
		VkCommandBufferAllocateInfo allocateInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			nullptr,
			mCommandPool,
			Map(createInfo.level),
			1};
		if (vkAllocateCommandBuffers(mDevice, &allocateInfo, &mHandle) != VK_SUCCESS)
			THROW("failed to create command buffer");
	}
	void VKCommandBuffer::Reset(CommandBufferResetFlatBits flags)
	{
		vkResetCommandBuffer(mHandle, Map(flags));
	}
	void VKCommandBuffer::Begin(const CommandBufferBeginInfo &beginInfo)
	{
		VkCommandBufferUsageFlags usage{};
		if (beginInfo.usage == CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT)
			usage |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		if (mCreateInfo.level == CommandBufferLevel::SECONDARY)
			usage |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

		VkCommandBufferInheritanceInfo inheritancInfo{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
		};
		if (beginInfo.pFramebuffer)
			inheritancInfo.framebuffer = static_cast<VKFramebuffer *>(beginInfo.pFramebuffer)->GetHandle();
		VkCommandBufferBeginInfo info{
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			nullptr,
			usage,
			&inheritancInfo};
		vkBeginCommandBuffer(mHandle, &info);
	}
	void VKCommandBuffer::End()
	{
		vkEndCommandBuffer(mHandle);
	}
	void VKCommandBuffer::ExecuteSecondaryCommandBuffer(const std::vector<CommandBuffer *> &secondaryCommandBuffers)
	{
		std::vector<VkCommandBuffer> cmdBuffers;
		for (auto &&e : secondaryCommandBuffers)
			cmdBuffers.emplace_back(static_cast<VKCommandBuffer *>(e)->GetHandle());
		vkCmdExecuteCommands(mHandle,
							 static_cast<uint32_t>(cmdBuffers.size()),
							 cmdBuffers.data());
	}
	void VKCommandBuffer::BeginRenderPass(const RenderPassBeginInfo &beginInfo, const SubpassBeginInfo &subpassBeginInfo)
	{
		auto count = beginInfo.clearValues.size();
		std::vector<VkClearValue> clearValues(count);
		while (count-- > 0)
		{
			memcpy(&clearValues[count], &beginInfo.clearValues[count], sizeof(VkClearValue));
		}
		VkRenderPassBeginInfo info{
			VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			nullptr,
			static_cast<VKRenderPass *>(beginInfo.pRenderPass)->GetHandle(),
			static_cast<VKFramebuffer *>(beginInfo.pFramebuffer)->GetHandle(),
			{},
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data()};
		memcpy(&info.renderArea, &beginInfo.renderArea, sizeof(VkRect2D));

		vkCmdBeginRenderPass(mHandle, &info, Map(subpassBeginInfo.contents));
	}
	void VKCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(mHandle);
	}
	void VKCommandBuffer::NextSubpass(const SubpassBeginInfo &subpassBeginInfo)
	{
		vkCmdNextSubpass(mHandle, Map(subpassBeginInfo.contents));
	}
	void VKCommandBuffer::BindPipeline(PipelineBindPoint bindPoint, Pipeline *pPipeline)
	{
		vkCmdBindPipeline(mHandle, Map(bindPoint), dynamic_cast<VKPipeline *>(pPipeline)->GetHandle());
	}
	void VKCommandBuffer::CopyBuffer(const CopyBufferInfo &copyInfo)
	{
		vkCmdCopyBuffer(
			mHandle,
			static_cast<VKBuffer *>(copyInfo.pSrcBuffer)->GetHandle(),
			static_cast<VKBuffer *>(copyInfo.pDstBuffer)->GetHandle(),
			static_cast<uint32_t>(copyInfo.regions.size()),
			reinterpret_cast<const VkBufferCopy *>(copyInfo.regions.data()));
	}
	void VKCommandBuffer::CopyImage(const CopyImageInfo &copyInfo)
	{
		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(copyInfo.pSrcImage->GetCreateInfoPtr()->format);

		std::vector<VkImageCopy> regions;
		for (auto &&e : copyInfo.regions)
		{
			regions.emplace_back(
				VkImageCopy{VkImageSubresourceLayers{
								aspectFlags,
								e.srcSubresource.mipLevel,
								e.srcSubresource.baseArrayLayer,
								e.srcSubresource.layerCount,
							},
							VkOffset3D{
								e.srcOffset.x,
								e.srcOffset.y,
								e.srcOffset.z,
							},
							VkImageSubresourceLayers{
								aspectFlags,
								e.dstSubresource.mipLevel,
								e.dstSubresource.baseArrayLayer,
								e.dstSubresource.layerCount,
							},
							VkOffset3D{
								e.dstOffset.x,
								e.dstOffset.y,
								e.dstOffset.z,
							},
							VkExtent3D{
								e.extent.width,
								e.extent.height,
								e.extent.depth,
							}});
		}
		vkCmdCopyImage(
			mHandle,
			static_cast<VKImage *>(copyInfo.pSrcImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			static_cast<VKImage *>(copyInfo.pDstImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()),
			regions.data());
	}
	void VKCommandBuffer::CopyBufferToImage(const CopyBufferToImageInfo &copyInfo)
	{
		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(copyInfo.pDstImage->GetCreateInfoPtr()->format);
		std::vector<VkBufferImageCopy> regions;
		for (auto &&e : copyInfo.regions)
		{
			regions.emplace_back(
				VkBufferImageCopy{e.bufferOffset,
								  e.bufferRowLength,
								  e.bufferImageHeight,
								  VkImageSubresourceLayers{
									  aspectFlags,
									  e.imageSubresource.mipLevel,
									  e.imageSubresource.baseArrayLayer,
									  e.imageSubresource.layerCount,
								  },
								  VkOffset3D{
									  e.imageOffset.x,
									  e.imageOffset.y,
									  e.imageOffset.z,
								  },
								  VkExtent3D{
									  e.imageExtent.width,
									  e.imageExtent.height,
									  e.imageExtent.depth,
								  }});
		}
		vkCmdCopyBufferToImage(
			mHandle,
			static_cast<VKBuffer *>(copyInfo.pSrcBuffer)->GetHandle(),
			static_cast<VKImage *>(copyInfo.pDstImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()),
			regions.data());
	}
	void VKCommandBuffer::CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo)
	{
		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(copyInfo.pSrcImage->GetCreateInfoPtr()->format);

		std::vector<VkBufferImageCopy> regions;
		for (auto &&e : copyInfo.regions)
		{
			regions.emplace_back(
				VkBufferImageCopy{e.bufferOffset,
								  e.bufferRowLength,
								  e.bufferImageHeight,
								  VkImageSubresourceLayers{
									  aspectFlags,
									  e.imageSubresource.mipLevel,
									  e.imageSubresource.baseArrayLayer,
									  e.imageSubresource.layerCount,
								  },
								  VkOffset3D{
									  e.imageOffset.x,
									  e.imageOffset.y,
									  e.imageOffset.z,
								  },
								  VkExtent3D{
									  e.imageExtent.width,
									  e.imageExtent.height,
									  e.imageExtent.depth,
								  }});
		}

		vkCmdCopyImageToBuffer(
			mHandle,
			static_cast<VKImage *>(copyInfo.pSrcImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<VKBuffer *>(copyInfo.pDstBuffer)->GetHandle(),
			static_cast<uint32_t>(regions.size()),
			regions.data());
	}
	void VKCommandBuffer::BlitImage(const BlitImageInfo &blitInfo)
	{
		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(blitInfo.pSrcImage->GetCreateInfoPtr()->format);

		std::vector<VkImageBlit> regions;
		VkImageBlit blit;
		for (auto &&e : blitInfo.regions)
		{
			blit.srcSubresource = {
				aspectFlags,
				e.srcSubresource.mipLevel,
				e.srcSubresource.baseArrayLayer,
				e.srcSubresource.layerCount,
			};
			blit.dstSubresource = {
				aspectFlags,
				e.dstSubresource.mipLevel,
				e.dstSubresource.baseArrayLayer,
				e.dstSubresource.layerCount,
			};
			memcpy(blit.srcOffsets, e.srcOffsets.data(), sizeof(VkOffset3D) * 2);
			memcpy(blit.dstOffsets, e.dstOffsets.data(), sizeof(VkOffset3D) * 2);
			regions.emplace_back(blit);
		}
		vkCmdBlitImage(
			mHandle,
			static_cast<VKImage *>(blitInfo.pSrcImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			static_cast<VKImage *>(blitInfo.pDstImage)->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(regions.size()),
			regions.data(),
			Map(blitInfo.filter));
	}
	void VKCommandBuffer::BindVertexBuffer(const BindVertexBufferInfo &info)
	{
		std::vector<VkBuffer> buffers;
		for (auto &&e : info.buffers)
			buffers.emplace_back(static_cast<VKBuffer *>(e)->GetHandle());
		vkCmdBindVertexBuffers(
			mHandle,
			info.firstBinding,
			static_cast<uint32_t>(info.buffers.size()),
			buffers.data(),
			info.offsets.data());
	}
	void VKCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info)
	{
		vkCmdBindIndexBuffer(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			Map(info.indexType));
	}
	void VKCommandBuffer::Draw(const DrawIndirectCommand &info)
	{
		vkCmdDraw(
			mHandle,
			info.vertexCount,
			info.instanceCount,
			info.firstVertex,
			info.firstInstance);
	}
	void VKCommandBuffer::DrawIndirect(const DrawIndirectInfo &info)
	{
		vkCmdDrawIndirect(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			info.drawCount,
			info.stride);
	}
	void VKCommandBuffer::DrawIndirectCount(const DrawIndirectCountInfo &info)
	{
#if SHIT_VK_VERSION_ATLEAST(1, 2, 0)
		vkCmdDrawIndirectCount(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			static_cast<VKBuffer *>(info.pCountBuffer)->GetHandle(),
			info.countBufferOffset,
			info.maxDrawCount,
			info.stride);
#else
	//exenstion
	THROW("draw indirect count is not supported");
#endif
	}
	void VKCommandBuffer::DrawIndexed(const DrawIndexedIndirectCommand &info)
	{
		vkCmdDrawIndexed(
			mHandle,
			info.indexCount,
			info.instanceCount,
			info.firstIndex,
			info.vertexOffset,
			info.firstInstance);
	}
	void VKCommandBuffer::DrawIndexedIndirect(const DrawIndirectInfo &info)
	{
		vkCmdDrawIndexedIndirect(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			info.drawCount,
			info.stride);
	}
	void VKCommandBuffer::DrawIndexedIndirectCount(const DrawIndirectCountInfo &info)
	{
#if SHIT_VK_VERSION_ATLEAST(1, 2, 0)
		vkCmdDrawIndexedIndirectCount(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			static_cast<VKBuffer *>(info.pCountBuffer)->GetHandle(),
			info.countBufferOffset,
			info.maxDrawCount,
			info.stride);
#else
#endif
	}

} // namespace Shi
