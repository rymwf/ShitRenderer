/**
 * @file VKCommandBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKCommandBuffer.hpp"
#include "VKBuffer.hpp"
#include "VKImage.hpp"
#include "VKFramebuffer.hpp"
#include "VKRenderPass.hpp"
#include "VKPipeline.hpp"
#include "VKBuffer.hpp"
#include "VKDescriptor.hpp"
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
	void VKCommandBuffer::ExecuteSecondaryCommandBuffer(const ExecuteSecondaryCommandBufferInfo &secondaryCommandBufferInfo)
	{
		std::vector<VkCommandBuffer> cmdBuffers;
		for (uint32_t i = 0; i < secondaryCommandBufferInfo.count; ++i)
			cmdBuffers.emplace_back(static_cast<VKCommandBuffer *>(&secondaryCommandBufferInfo.pCommandBuffers[i])->GetHandle());
		vkCmdExecuteCommands(mHandle,
							 static_cast<uint32_t>(cmdBuffers.size()),
							 cmdBuffers.data());
	}
	void VKCommandBuffer::BeginRenderPass(const RenderPassBeginInfo &beginInfo)
	{
		auto count = beginInfo.clearValueCount;
		std::vector<VkClearValue> clearValues(count);
		while (count-- > 0)
		{
			memcpy(&clearValues[count], &beginInfo.pClearValues[count], sizeof(VkClearValue));
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

		vkCmdBeginRenderPass(mHandle, &info, Map(beginInfo.contents));
	}
	void VKCommandBuffer::EndRenderPass()
	{
		vkCmdEndRenderPass(mHandle);
	}
	void VKCommandBuffer::NextSubpass(SubpassContents subpassContents)
	{
		vkCmdNextSubpass(mHandle, Map(subpassContents));
	}
	void VKCommandBuffer::BindPipeline(const BindPipelineInfo &info)
	{
		auto pPipeline = dynamic_cast<VKPipeline *>(info.pPipeline);
		if (pPipeline)
			vkCmdBindPipeline(mHandle, Map(info.bindPoint), pPipeline->GetHandle());
		else
			THROW("pipeline is null");
	}
	void VKCommandBuffer::CopyBuffer(const CopyBufferInfo &copyInfo)
	{
		vkCmdCopyBuffer(
			mHandle,
			static_cast<VKBuffer *>(copyInfo.pSrcBuffer)->GetHandle(),
			static_cast<VKBuffer *>(copyInfo.pDstBuffer)->GetHandle(),
			copyInfo.regionCount,
			reinterpret_cast<const VkBufferCopy *>(copyInfo.pRegions));
	}
	void VKCommandBuffer::CopyImage(const CopyImageInfo &copyInfo)
	{
		VkImageAspectFlags aspectFlags = GetImageAspectFromFormat(copyInfo.pSrcImage->GetCreateInfoPtr()->format);

		std::vector<VkImageCopy> regions;
		for (uint32_t i = 0; i < copyInfo.regionCount; ++i)
		{
			regions.emplace_back(
				VkImageCopy{VkImageSubresourceLayers{
								aspectFlags,
								copyInfo.pRegions[i].srcSubresource.mipLevel,
								copyInfo.pRegions[i].srcSubresource.baseArrayLayer,
								copyInfo.pRegions[i].srcSubresource.layerCount,
							},
							VkOffset3D{
								copyInfo.pRegions[i].srcOffset.x,
								copyInfo.pRegions[i].srcOffset.y,
								copyInfo.pRegions[i].srcOffset.z,
							},
							VkImageSubresourceLayers{
								aspectFlags,
								copyInfo.pRegions[i].dstSubresource.mipLevel,
								copyInfo.pRegions[i].dstSubresource.baseArrayLayer,
								copyInfo.pRegions[i].dstSubresource.layerCount,
							},
							VkOffset3D{
								copyInfo.pRegions[i].dstOffset.x,
								copyInfo.pRegions[i].dstOffset.y,
								copyInfo.pRegions[i].dstOffset.z,
							},
							VkExtent3D{
								copyInfo.pRegions[i].extent.width,
								copyInfo.pRegions[i].extent.height,
								copyInfo.pRegions[i].extent.depth,
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
		for (uint32_t i = 0; i < copyInfo.regionCount; ++i)
		{
			regions.emplace_back(
				VkBufferImageCopy{copyInfo.pRegions[i].bufferOffset,
								  copyInfo.pRegions[i].bufferRowLength,
								  copyInfo.pRegions[i].bufferImageHeight,
								  VkImageSubresourceLayers{
									  aspectFlags,
									  copyInfo.pRegions[i].imageSubresource.mipLevel,
									  copyInfo.pRegions[i].imageSubresource.baseArrayLayer,
									  copyInfo.pRegions[i].imageSubresource.layerCount,
								  },
								  VkOffset3D{
									  copyInfo.pRegions[i].imageOffset.x,
									  copyInfo.pRegions[i].imageOffset.y,
									  copyInfo.pRegions[i].imageOffset.z,
								  },
								  VkExtent3D{
									  copyInfo.pRegions[i].imageExtent.width,
									  copyInfo.pRegions[i].imageExtent.height,
									  copyInfo.pRegions[i].imageExtent.depth,
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
		for (uint32_t i = 0; i < copyInfo.regionCount; ++i)
		{
			regions.emplace_back(
				VkBufferImageCopy{copyInfo.pRegions[i].bufferOffset,
								  copyInfo.pRegions[i].bufferRowLength,
								  copyInfo.pRegions[i].bufferImageHeight,
								  VkImageSubresourceLayers{
									  aspectFlags,
									  copyInfo.pRegions[i].imageSubresource.mipLevel,
									  copyInfo.pRegions[i].imageSubresource.baseArrayLayer,
									  copyInfo.pRegions[i].imageSubresource.layerCount,
								  },
								  VkOffset3D{
									  copyInfo.pRegions[i].imageOffset.x,
									  copyInfo.pRegions[i].imageOffset.y,
									  copyInfo.pRegions[i].imageOffset.z,
								  },
								  VkExtent3D{
									  copyInfo.pRegions[i].imageExtent.width,
									  copyInfo.pRegions[i].imageExtent.height,
									  copyInfo.pRegions[i].imageExtent.depth,
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
		for (uint32_t i = 0; i < blitInfo.regionCount; ++i)
		{
			blit.srcSubresource = {
				aspectFlags,
				blitInfo.pRegions[i].srcSubresource.mipLevel,
				blitInfo.pRegions[i].srcSubresource.baseArrayLayer,
				blitInfo.pRegions[i].srcSubresource.layerCount,
			};
			blit.dstSubresource = {
				aspectFlags,
				blitInfo.pRegions[i].dstSubresource.mipLevel,
				blitInfo.pRegions[i].dstSubresource.baseArrayLayer,
				blitInfo.pRegions[i].dstSubresource.layerCount,
			};
			memcpy(blit.srcOffsets, blitInfo.pRegions[i].srcOffsets.data(), sizeof(VkOffset3D) * 2);
			memcpy(blit.dstOffsets, blitInfo.pRegions[i].dstOffsets.data(), sizeof(VkOffset3D) * 2);
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
		for (size_t i = 0; i < info.bindingCount; ++i)
			buffers.emplace_back(static_cast<const VKBuffer *>(info.ppBuffers[i])->GetHandle());
		vkCmdBindVertexBuffers(
			mHandle,
			info.firstBinding,
			info.bindingCount,
			buffers.data(),
			info.pOffsets);
	}
	void VKCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info)
	{
		vkCmdBindIndexBuffer(
			mHandle,
			static_cast<VKBuffer *>(info.pBuffer)->GetHandle(),
			info.offset,
			Map(info.indexType));
	}
	void VKCommandBuffer::BindDescriptorSets(const BindDescriptorSetsInfo &info)
	{
		std::vector<VkDescriptorSet> descriptorSets(info.descriptorSetCount);
		for (uint32_t i = 0; i < info.descriptorSetCount; ++i)
		{
			descriptorSets[i] = static_cast<const VKDescriptorSet *>(info.ppDescriptorSets[i])->GetHandle();
		}
		std::vector<uint32_t> dynamicOffsets(info.dynamicOffsetCount);
		for (uint32_t i = 0; i < info.dynamicOffsetCount; ++i)
		{
			dynamicOffsets[i] = info.pDynamicOffsets[i];
		}

		vkCmdBindDescriptorSets(mHandle,
								Map(info.pipelineBindPoint),
								static_cast<VKPipelineLayout *>(info.pPipelineLayout)->GetHandle(),
								info.firstset,
								info.descriptorSetCount,
								descriptorSets.data(),
								info.dynamicOffsetCount,
								dynamicOffsets.data());
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
	void VKCommandBuffer::DrawIndirectCount([[maybe_unused]] const DrawIndirectCountInfo &info)
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
		THROW("vulkan 1.2 is needed to  support drawIndirectCount");
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
	void VKCommandBuffer::DrawIndexedIndirectCount([[maybe_unused]] const DrawIndirectCountInfo &info)
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
		THROW("vulkan 1.2 is needed to  support drawIndexedIndirectCount");
#endif
	}

	void VKCommandBuffer::PipeplineBarrier(const PipelineBarrierInfo &info)
	{
		std::vector<VkMemoryBarrier> memoryBarriers(info.memoryBarriers.size());
		std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers(info.bufferMemoryBarriers.size());
		std::vector<VkImageMemoryBarrier> imageMemoryBarriers(info.imageMemoryBarriers.size());
		std::transform(std::execution::par, info.memoryBarriers.begin(), info.memoryBarriers.end(), memoryBarriers.begin(), [](auto &&e) {
			return VkMemoryBarrier{
				VK_STRUCTURE_TYPE_MEMORY_BARRIER,
				nullptr,
				Map(e.srcAccessMask),
				Map(e.dstAccessMask)};
		});
		std::transform(std::execution::par, info.bufferMemoryBarriers.begin(), info.bufferMemoryBarriers.end(), bufferMemoryBarriers.begin(), [](auto &&e) {
			return VkBufferMemoryBarrier{
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
				nullptr,
				Map(e.srcAccessMask),
				Map(e.dstAccessMask),
				VK_QUEUE_FAMILY_IGNORED,//e.srcQueueFamilyIndex,
				VK_QUEUE_FAMILY_IGNORED,//e.dstQueueFamilyIndex,
				static_cast<VKBuffer *>(e.pBuffer)->GetHandle(),
				e.offset,
				e.size};
		});
		std::transform(std::execution::par, info.imageMemoryBarriers.begin(), info.imageMemoryBarriers.end(), imageMemoryBarriers.begin(), [](auto &&e) {
			return VkImageMemoryBarrier{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				Map(e.srcAccessMask),
				Map(e.dstAccessMask),
				Map(e.oldImageLayout),
				Map(e.newImageLayout),
				VK_QUEUE_FAMILY_IGNORED,//e.srcQueueFamilyIndex,
				VK_QUEUE_FAMILY_IGNORED,//e.dstQueueFamilyIndex,
				static_cast<VKImage *>(e.pImage)->GetHandle(),
				VkImageSubresourceRange{
					GetImageAspectFromFormat(static_cast<VKImage *>(e.pImage)->GetCreateInfoPtr()->format),
					e.subresourceRange.baseMipLevel,
					e.subresourceRange.levelCount,
					e.subresourceRange.baseArrayLayer,
					e.subresourceRange.layerCount,
				}};
		});
		vkCmdPipelineBarrier(
			mHandle,
			Map(info.srcStageMask),
			Map(info.dstStageMask),
			Map(info.dependencyFlags),
			static_cast<uint32_t>(memoryBarriers.size()),
			memoryBarriers.data(),
			static_cast<uint32_t>(bufferMemoryBarriers.size()),
			bufferMemoryBarriers.data(),
			static_cast<uint32_t>(imageMemoryBarriers.size()),
			imageMemoryBarriers.data());
	}
} // namespace Shi
