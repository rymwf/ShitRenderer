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
#include "VKBufferView.hpp"
#include "VKCommandPool.hpp"
#include "VKDescriptor.hpp"
#include "VKFramebuffer.hpp"
#include "VKImage.hpp"
#include "VKPipeline.hpp"
#include "VKRenderPass.hpp"
#include "VKSampler.hpp"

namespace Shit {
VKCommandBuffer::VKCommandBuffer(VKDevice *pDevice, VKCommandPool *pCommandPool, CommandBufferLevel level)
    : CommandBuffer(pCommandPool, level), VKDeviceObject(pDevice) {
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr,
                                             pCommandPool->GetHandle(), Map(level), 1};
    CHECK_VK_RESULT(vkAllocateCommandBuffers(mpDevice->GetHandle(), &allocateInfo, &mHandle))
}
void VKCommandBuffer::Destroy() {
    vkFreeCommandBuffers(mpDevice->GetHandle(), static_cast<VKCommandPool *>(mCommandPool)->GetHandle(), 1, &mHandle);
}
void VKCommandBuffer::Reset(CommandBufferResetFlatBits flags) { vkResetCommandBuffer(mHandle, Map(flags)); }
void VKCommandBuffer::Begin(const CommandBufferBeginInfo &beginInfo) {
    VkCommandBufferInheritanceInfo inheritancInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        nullptr,
        beginInfo.inheritanceInfo.pRenderPass
            ? static_cast<VKRenderPass const *>(beginInfo.inheritanceInfo.pRenderPass)->GetHandle()
            : VK_NULL_HANDLE,
        beginInfo.inheritanceInfo.subpass,
        beginInfo.inheritanceInfo.pFramebuffer
            ? static_cast<VKFramebuffer const *>(beginInfo.inheritanceInfo.pFramebuffer)->GetHandle()
            : VK_NULL_HANDLE,
    };
    VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr, Map(beginInfo.usage),
                                  &inheritancInfo};
    vkBeginCommandBuffer(mHandle, &info);
}
void VKCommandBuffer::End() { vkEndCommandBuffer(mHandle); }
void VKCommandBuffer::ExecuteCommands(const ExecuteCommandsInfo &info) {
    std::vector<VkCommandBuffer> cmdBuffers;
    std::ranges::transform(info.pCommandBuffers, info.pCommandBuffers + info.count, std::back_inserter(cmdBuffers),
                           [](auto p) { return static_cast<const VKCommandBuffer *>(p)->GetHandle(); });
    vkCmdExecuteCommands(mHandle, static_cast<uint32_t>(cmdBuffers.size()), cmdBuffers.data());
}
void VKCommandBuffer::BeginRenderPass(const BeginRenderPassInfo &beginInfo) {
    std::vector<VkClearValue> clearValues(beginInfo.clearValueCount);
    for (uint32_t i = 0; i < beginInfo.clearValueCount; ++i) {
        memcpy(&clearValues[i], &beginInfo.pClearValues[i], sizeof(VkClearValue));
    }
    VkRenderPassAttachmentBeginInfo attachmentBeginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO, 0,
                                                        beginInfo.attachmentBeginInfo.attachmentCount};
    std::vector<VkImageView> pImageViews(beginInfo.attachmentBeginInfo.attachmentCount);
    for (uint32_t i = 0; i < beginInfo.attachmentBeginInfo.attachmentCount; ++i) {
        pImageViews[i] = static_cast<VKImageView const *>(beginInfo.attachmentBeginInfo.pAttachments[i])->GetHandle();
    }
    attachmentBeginInfo.pAttachments = pImageViews.data();

    VkRenderPassBeginInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                               &attachmentBeginInfo,
                               static_cast<VKRenderPass const *>(beginInfo.pRenderPass)->GetHandle(),
                               static_cast<VKFramebuffer const *>(beginInfo.pFramebuffer)->GetHandle(),
                               *reinterpret_cast<const VkRect2D *>(&beginInfo.renderArea),
                               beginInfo.clearValueCount,
                               clearValues.data()};
    // VkRenderPassBeginInfo info{
    //	VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    //	nullptr,
    //	static_cast<VKRenderPass const *>(beginInfo.pRenderPass)->GetHandle(),
    //	static_cast<VKFramebuffer const *>(beginInfo.pFramebuffer)->GetHandle(),
    //	*reinterpret_cast<const VkRect2D *>(&beginInfo.renderArea),
    //	beginInfo.clearValueCount,
    //	reinterpret_cast<const VkClearValue *>(beginInfo.pClearValues)};
    vkCmdBeginRenderPass(mHandle, &info, Map(beginInfo.contents));
}
void VKCommandBuffer::EndRenderPass() { vkCmdEndRenderPass(mHandle); }
void VKCommandBuffer::NextSubpass(SubpassContents subpassContents) { vkCmdNextSubpass(mHandle, Map(subpassContents)); }
void VKCommandBuffer::BindPipeline(const BindPipelineInfo &info) {
    auto pPipeline = dynamic_cast<VKPipeline const *>(info.pPipeline);
    if (pPipeline)
        vkCmdBindPipeline(mHandle, Map(info.bindPoint), pPipeline->GetHandle());
    else
        ST_THROW("pipeline is null")
}
void VKCommandBuffer::CopyBuffer(const CopyBufferInfo &copyInfo) {
    vkCmdCopyBuffer(mHandle, static_cast<VKBuffer const *>(copyInfo.pSrcBuffer)->GetHandle(),
                    static_cast<VKBuffer const *>(copyInfo.pDstBuffer)->GetHandle(), copyInfo.regionCount,
                    reinterpret_cast<const VkBufferCopy *>(copyInfo.pRegions));
}
void VKCommandBuffer::CopyImage(const CopyImageInfo &copyInfo) {
    vkCmdCopyImage(mHandle, static_cast<VKImage const *>(copyInfo.pSrcImage)->GetHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<VKImage const *>(copyInfo.pDstImage)->GetHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyInfo.regionCount,
                   reinterpret_cast<const VkImageCopy *>(copyInfo.pRegions));
}
void VKCommandBuffer::CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) {
    vkCmdCopyBufferToImage(mHandle, static_cast<VKBuffer const *>(copyInfo.pSrcBuffer)->GetHandle(),
                           static_cast<VKImage const *>(copyInfo.pDstImage)->GetHandle(),
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, copyInfo.regionCount,
                           reinterpret_cast<const VkBufferImageCopy *>(copyInfo.pRegions));
}
void VKCommandBuffer::CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) {
    vkCmdCopyImageToBuffer(mHandle, static_cast<VKImage const *>(copyInfo.pSrcImage)->GetHandle(),
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           static_cast<VKBuffer const *>(copyInfo.pDstBuffer)->GetHandle(), copyInfo.regionCount,
                           reinterpret_cast<const VkBufferImageCopy *>(copyInfo.pRegions));
}
void VKCommandBuffer::BlitImage(const BlitImageInfo &blitInfo) {
    vkCmdBlitImage(mHandle, static_cast<VKImage const *>(blitInfo.pSrcImage)->GetHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, static_cast<VKImage const *>(blitInfo.pDstImage)->GetHandle(),
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, blitInfo.regionCount,
                   reinterpret_cast<const VkImageBlit *>(blitInfo.pRegions), Map(blitInfo.filter));
}
void VKCommandBuffer::BindVertexBuffers(const BindVertexBuffersInfo &info) {
    std::vector<VkBuffer> buffers;
    static auto vt = std::views::transform([](Buffer const *pBuffer) {
        return pBuffer ? static_cast<VKBuffer const *>(pBuffer)->GetHandle() : VK_NULL_HANDLE;
    });
    buffers.reserve(info.bindingCount);
    std::ranges::copy(std::span(info.ppBuffers, info.bindingCount) | vt, std::back_inserter(buffers));

    // if (mpDevice->IsExtensionSupported("VK_EXT_robustness2"))
    // {
    // 	//TODO: check if nulldescriptor feature enabled
    // 	vkCmdBindVertexBuffers(
    // 		mHandle,
    // 		info.firstBinding,
    // 		info.bindingCount,
    // 		buffers.data(),
    // 		info.pOffsets);
    // }
    // else
    {
        auto beg = std::ranges::cbegin(buffers);
        auto it0 = beg, it1 = beg;
        auto end = std::ranges::cend(buffers);
        for (; it1 != end; ++it1) {
            if (*it1 == VK_NULL_HANDLE) {
                if (it1 != it0) {
                    uint32_t offset = static_cast<uint32_t>(std::distance(beg, it0));
                    vkCmdBindVertexBuffers(mHandle, info.firstBinding + offset,
                                           static_cast<uint32_t>(std::distance(it0, it1)), &buffers[offset],
                                           &info.pOffsets[offset]);
                }
                it0 = it1;
                ++it0;
            }
        }
        if (it1 != it0) {
            uint32_t offset = static_cast<uint32_t>(std::distance(beg, it0));
            vkCmdBindVertexBuffers(mHandle, info.firstBinding + offset, static_cast<uint32_t>(std::distance(it0, it1)),
                                   &buffers[offset], &info.pOffsets[offset]);
        }
    }
}
void VKCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info) {
    vkCmdBindIndexBuffer(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset,
                         Map(info.indexType));
}
void VKCommandBuffer::BindDescriptorSets(const BindDescriptorSetsInfo &info) {
    std::vector<VkDescriptorSet> descriptorSets;
    std::ranges::transform(info.ppDescriptorSets, info.ppDescriptorSets + info.descriptorSetCount,
                           std::back_inserter(descriptorSets),
                           [](auto p) { return static_cast<const VKDescriptorSet *>(p)->GetHandle(); });
    vkCmdBindDescriptorSets(
        mHandle, Map(info.pipelineBindPoint),
        static_cast<VKPipelineLayout const *>(info.pPipelineLayout)->GetHandle(),  // cannot be null handle
        info.firstset, info.descriptorSetCount, descriptorSets.data(), info.dynamicOffsetCount, info.pDynamicOffsets);
}
void VKCommandBuffer::Draw(const DrawIndirectCommand &info) {
    vkCmdDraw(mHandle, info.vertexCount, info.instanceCount, info.firstVertex, info.firstInstance);
}
void VKCommandBuffer::Draw(const DrawIndexedIndirectCommand &info) { DrawIndexed(info); }
void VKCommandBuffer::DrawIndirect(const DrawIndirectInfo &info) {
    vkCmdDrawIndirect(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset, info.drawCount,
                      info.stride);
}
void VKCommandBuffer::DrawIndirect(const DrawIndirectCountInfo &info) { DrawIndirectCount(info); }
void VKCommandBuffer::DrawIndirectCount([[maybe_unused]] const DrawIndirectCountInfo &info) {
#if SHIT_VK_VERSION_ATLEAST(1, 2)
    vkCmdDrawIndirectCount(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset,
                           static_cast<VKBuffer const *>(info.pCountBuffer)->GetHandle(), info.countBufferOffset,
                           info.maxDrawCount, info.stride);
#else
    // exenstion
    ST_THROW("vulkan 1.2 is needed to  support drawIndirectCount")
#endif
}
void VKCommandBuffer::DrawIndexed(const DrawIndexedIndirectCommand &info) {
    vkCmdDrawIndexed(mHandle, info.indexCount, info.instanceCount, info.firstIndex, info.vertexOffset,
                     info.firstInstance);
}
void VKCommandBuffer::DrawIndexedIndirect(const DrawIndirectInfo &info) {
    vkCmdDrawIndexedIndirect(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset,
                             info.drawCount, info.stride);
}
void VKCommandBuffer::DrawIndexedIndirect(const DrawIndirectCountInfo &info) { DrawIndexedIndirectCount(info); }
void VKCommandBuffer::DrawIndexedIndirectCount([[maybe_unused]] const DrawIndirectCountInfo &info) {
#if SHIT_VK_VERSION_ATLEAST(1, 2)
    vkCmdDrawIndexedIndirectCount(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset,
                                  static_cast<VKBuffer const *>(info.pCountBuffer)->GetHandle(), info.countBufferOffset,
                                  info.maxDrawCount, info.stride);
#else
    ST_THROW("vulkan 1.2 is needed to  support drawIndexedIndirectCount")
#endif
}
void VKCommandBuffer::ClearColorImage(const ClearColorImageInfo &info) {
    vkCmdClearColorImage(mHandle, static_cast<VKImage const *>(info.image)->GetHandle(), Map(info.imageLayout),
                         reinterpret_cast<const VkClearColorValue *>(&info.clearColor), info.rangeCount,
                         reinterpret_cast<const VkImageSubresourceRange *>(info.pRanges));
}
void VKCommandBuffer::ClearDepthStencilImage(const ClearDepthStencilImageInfo &info) {
    vkCmdClearDepthStencilImage(mHandle, static_cast<VKImage const *>(info.image)->GetHandle(), Map(info.imageLayout),
                                reinterpret_cast<const VkClearDepthStencilValue *>(&info.depthStencil), info.rangeCount,
                                reinterpret_cast<const VkImageSubresourceRange *>(info.pRanges));
}
void VKCommandBuffer::ClearAttachments(const ClearAttachmentsInfo &info) {
    vkCmdClearAttachments(mHandle, info.attachmentCount, reinterpret_cast<const VkClearAttachment *>(info.pAttachments),
                          info.rectCount, reinterpret_cast<const VkClearRect *>(info.pRects));
}
void VKCommandBuffer::FillBuffer(const FillBufferInfo &info) {
    vkCmdFillBuffer(mHandle, static_cast<VKBuffer const *>(info.buffer)->GetHandle(), info.offset, info.size,
                    info.data);
}
void VKCommandBuffer::UpdateBuffer(const UpdateBufferInfo &info) {
    vkCmdUpdateBuffer(mHandle, static_cast<VKBuffer const *>(info.dstBuffer)->GetHandle(), info.dstOffset,
                      info.dataSize, info.pData);
}
void VKCommandBuffer::PipelineBarrier(const PipelineBarrierInfo &info) {
    std::vector<VkMemoryBarrier> memoryBarriers;
    std::vector<VkBufferMemoryBarrier> bufferMemoryBarriers;
    std::vector<VkImageMemoryBarrier> imageMemoryBarriers;

    std::ranges::transform(info.pMemoryBarriers, info.pMemoryBarriers + info.memoryBarrierCount,
                           std::back_inserter(memoryBarriers), [](auto &&e) {
                               return VkMemoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr, Map(e.srcAccessMask),
                                                      Map(e.dstAccessMask)};
                           });
    std::ranges::transform(info.pBufferMemoryBarriers, info.pBufferMemoryBarriers + info.bufferMemoryBarrierCount,
                           std::back_inserter(bufferMemoryBarriers), [](auto &&e) {
                               return VkBufferMemoryBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
                                                            nullptr,
                                                            Map(e.srcAccessMask),
                                                            Map(e.dstAccessMask),
                                                            e.srcQueueFamilyIndex,
                                                            e.dstQueueFamilyIndex,
                                                            static_cast<VKBuffer const *>(e.pBuffer)->GetHandle(),
                                                            e.offset,
                                                            e.size};
                           });
    std::ranges::transform(info.pImageMemoryBarriers, info.pImageMemoryBarriers + info.imageMemoryBarrierCount,
                           std::back_inserter(imageMemoryBarriers), [](auto &&e) {
                               return VkImageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                                           nullptr,
                                                           Map(e.srcAccessMask),
                                                           Map(e.dstAccessMask),
                                                           Map(e.oldImageLayout),
                                                           Map(e.newImageLayout),
                                                           e.srcQueueFamilyIndex,
                                                           e.dstQueueFamilyIndex,
                                                           static_cast<VKImage const *>(e.pImage)->GetHandle(),
                                                           VkImageSubresourceRange{
                                                               Map(e.subresourceRange.aspectMask),
                                                               e.subresourceRange.baseMipLevel,
                                                               e.subresourceRange.levelCount,
                                                               e.subresourceRange.baseArrayLayer,
                                                               e.subresourceRange.layerCount,
                                                           }};
                           });
    vkCmdPipelineBarrier(mHandle, Map(info.srcStageMask), Map(info.dstStageMask), Map(info.dependencyFlags),
                         static_cast<uint32_t>(memoryBarriers.size()), memoryBarriers.data(),
                         static_cast<uint32_t>(bufferMemoryBarriers.size()), bufferMemoryBarriers.data(),
                         static_cast<uint32_t>(imageMemoryBarriers.size()), imageMemoryBarriers.data());
}
void VKCommandBuffer::PushConstants(const PushConstantInfo &info) {
    vkCmdPushConstants(mHandle, static_cast<VKPipelineLayout const *>(info.pPipelineLayout)->GetHandle(),
                       Map(info.stageFlags), info.offset, info.size, info.pValues);
}
void VKCommandBuffer::PushDescriptorSet(const PushDescriptorSetInfo &info) {
    struct Temp {
        std::vector<VkDescriptorImageInfo> imagesInfo;
        std::vector<VkDescriptorBufferInfo> buffersInfo;
        std::vector<VkBufferView> texelBufferViews;
    };
    std::vector<VkWriteDescriptorSet> writes(info.descriptorWriteCount);
    std::vector<Temp> temp(info.descriptorWriteCount);

    for (uint32_t i = 0, j; i < info.descriptorWriteCount; ++i) {
        auto &&e = info.pDescriptorWrites[i];
        writes[i] = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            nullptr,
            static_cast<VKDescriptorSet const *>(e.pDstSet)->GetHandle(),
            e.dstBinding,
            e.dstArrayElement,
            e.descriptorCount,
            Map(e.descriptorType),
        };
        switch (e.descriptorType) {
            case DescriptorType::SAMPLER:  // sampler (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::COMBINED_IMAGE_SAMPLER:  // sampler2D
                ST_FALLTHROUGH;
            case DescriptorType::SAMPLED_IMAGE:  // texture2D (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_IMAGE:  // image2D
                ST_FALLTHROUGH;
            case DescriptorType::INPUT_ATTACHMENT: {
                for (j = 0; j < e.descriptorCount; ++j) {
                    temp[i].imagesInfo.emplace_back(
                        e.pImageInfo[j].pSampler ? static_cast<VKSampler const *>(e.pImageInfo[j].pSampler)->GetHandle()
                                                 : VK_NULL_HANDLE,
                        e.pImageInfo[j].pImageView
                            ? static_cast<VKImageView const *>(e.pImageInfo[j].pImageView)->GetHandle()
                            : VK_NULL_HANDLE,
                        Map(e.pImageInfo[j].imageLayout));
                }
                writes[i].pImageInfo = temp[i].imagesInfo.data();
                break;
            }
            case DescriptorType::UNIFORM_TEXEL_BUFFER:  // samplerbuffer	(access to buffer
                                                        // texture,can only be accessed with texelFetch
                                                        // function) ,textureBuffer(vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_TEXEL_BUFFER:  // imagebuffer (access to
                                                        // buffer texture)
            {
                for (j = 0; j < e.descriptorCount; ++j) {
                    temp[i].texelBufferViews.emplace_back(
                        static_cast<const VKBufferView *>(e.pTexelBufferView[j])->GetHandle());
                }
                writes[i].pTexelBufferView = temp[i].texelBufferViews.data();
                break;
            }
            case DescriptorType::UNIFORM_BUFFER:  // uniform block
            case DescriptorType::STORAGE_BUFFER:  // buffer block
            case DescriptorType::UNIFORM_BUFFER_DYNAMIC:
            case DescriptorType::STORAGE_BUFFER_DYNAMIC: {
                for (j = 0; j < e.descriptorCount; ++j) {
                    auto &&buffer = e.pBufferInfo[j];
                    temp[i].buffersInfo.emplace_back(
                        buffer.pBuffer ? static_cast<VKBuffer const *>(buffer.pBuffer)->GetHandle() : VK_NULL_HANDLE,
                        buffer.offset, buffer.range);
                }
                writes[i].pBufferInfo = temp[i].buffersInfo.data();
                break;
            }
        }
    }
#if 0
		vkCmdPushDescriptorSetKHR(
			mHandle,
			Map(info.pipelineBindPoint),
			static_cast<VKPipelineLayout const *>(info.pLayout)->GetHandle(),
			info.set,
			(uint32_t)writes.size(),
			writes.data());
#else
    ST_THROW("PushDescriptorSet not implemented yet")
#endif
}
void VKCommandBuffer::Dispatch(const DispatchInfo &info) {
    vkCmdDispatch(mHandle, info.groupCountX, info.groupCountY, info.groupCountZ);
}
void VKCommandBuffer::DispatchIndirect(const DispatchIndirectInfo &info) {
    vkCmdDispatchIndirect(mHandle, static_cast<VKBuffer const *>(info.pBuffer)->GetHandle(), info.offset);
}
void VKCommandBuffer::BindTransformFeedbackBuffers(const BindTransformFeedbackBuffersInfo &info) {
    std::vector<VkBuffer> buffers(info.bindingCount);
    for (uint32_t i = 0; i < info.bindingCount; ++i) {
        buffers[i] = static_cast<VKBuffer const *>(info.ppBuffers[i])->GetHandle();
    }
    // vkCmdBindTransformFeedbackBuffersEXT(
    //	mHandle,
    //	info.firstBinding,
    //	info.bindingCount,
    //	buffers.data(),
    //	info.pOffsets,
    //	info.pSizes);
}
void VKCommandBuffer::BeginTransformFeedback(const BeginTransformFeedbackInfo &info) {
    std::vector<VkBuffer> buffers(info.counterBufferCount);
    for (uint32_t i = 0; i < info.counterBufferCount; ++i) {
        buffers[i] = static_cast<VKBuffer const *>(info.ppCounterBuffers[i])->GetHandle();
    }
    // vkCmdBeginTransformFeedbackEXT(
    //	mHandle,
    //	info.firstCounterBuffer,
    //	info.counterBufferCount,
    //	buffers.data(),
    //	info.pCounterBufferOffsets);
}
void VKCommandBuffer::EndTransformFeedback(const EndTransformFeedbackInfo &info) {
    std::vector<VkBuffer> buffers(info.counterBufferCount);
    for (uint32_t i = 0; i < info.counterBufferCount; ++i) {
        buffers[i] = static_cast<VKBuffer const *>(info.ppCounterBuffers[i])->GetHandle();
    }
    // vkCmdEndTransformFeedbackEXT(
    //	mHandle,
    //	info.firstCounterBuffer,
    //	info.counterBufferCount,
    //	buffers.data(),
    //	info.pCounterBufferOffsets);
}
void VKCommandBuffer::SetViewport(const SetViewPortInfo &info) {
    std::vector<VkViewport> viewports(info.viewportCount);
    for (uint32_t i = 0; i < info.viewportCount; ++i)
        viewports[i] = {
            info.pViewports[i].x,        info.pViewports[i].height - info.pViewports[i].y,
            info.pViewports[i].width,    -info.pViewports[i].height,
            info.pViewports[i].minDepth, info.pViewports[i].maxDepth,
        };
    vkCmdSetViewport(mHandle, info.firstViewport, info.viewportCount, viewports.data());
}
void VKCommandBuffer::SetLineWidth(float lineWidth) { vkCmdSetLineWidth(mHandle, lineWidth); }
void VKCommandBuffer::SetDepthBias(const SetDepthBiasInfo &info) {
    vkCmdSetDepthBias(mHandle, info.depthBiasConstantFactor, info.depthBiasClamp, info.depthBiasSlopFactor);
}
void VKCommandBuffer::SetBlendConstants(const float blendConstants[4]) {
    vkCmdSetBlendConstants(mHandle, blendConstants);
}
void VKCommandBuffer::SetScissor(const SetScissorInfo &info) {
    vkCmdSetScissor(mHandle, info.firstScissor, info.scissorCount, reinterpret_cast<const VkRect2D *>(info.pScissors));
}
void VKCommandBuffer::SetDepthBounds(const SetDepthBoundsInfo &info) {
    vkCmdSetDepthBounds(mHandle, info.minDepthBounds, info.maxDepthBounds);
}
void VKCommandBuffer::SetStencilCompareMask(const SetStencilCompareMaskInfo &info) {
    vkCmdSetStencilCompareMask(mHandle, Map(info.faceMask), info.mask);
}
void VKCommandBuffer::SetStencilWriteMask(const SetStencilWriteMaskInfo &info) {
    vkCmdSetStencilWriteMask(mHandle, Map(info.faceMask), info.mask);
}
void VKCommandBuffer::SetStencilReference(const SetStencilReferenceInfo &info) {
    vkCmdSetStencilReference(mHandle, Map(info.faceMask), info.mask);
}
void VKCommandBuffer::ResolveImage(const ResolveImageInfo &info) {
    std::vector<VkImageResolve> imageResolves(info.regionCount);
    for (uint32_t i = 0; i < info.regionCount; ++i) {
        auto &&e = info.pRegions[i];
        imageResolves[i] = {{Map(e.srcSubresource.aspectMask), e.srcSubresource.mipLevel,
                             e.srcSubresource.baseArrayLayer, e.srcSubresource.layerCount},
                            {e.srcOffset.x, e.srcOffset.y, e.srcOffset.z},
                            {Map(e.dstSubresource.aspectMask), e.dstSubresource.mipLevel,
                             e.dstSubresource.baseArrayLayer, e.dstSubresource.layerCount},
                            {e.dstOffset.x, e.dstOffset.y, e.dstOffset.z},
                            {e.extent.width, e.extent.depth, e.extent.depth}};
    }
    vkCmdResolveImage(mHandle, static_cast<VKImage const *>(info.pSrcImage)->GetHandle(), Map(info.srcImageLayout),
                      static_cast<VKImage const *>(info.pDstImage)->GetHandle(), Map(info.dstImageLayout),
                      info.regionCount, imageResolves.data());
}
void VKCommandBuffer::GenerateMipmap(const GenerateMipmapInfo &info) {
    auto &&pImageCreateInfo = info.pImage->GetCreateInfoPtr();
    auto formatAttribute = GetFormatAttribute(pImageCreateInfo->format);
    ImageAspectFlagBits imageAspect;
    if (formatAttribute.baseFormat == BaseFormat::DEPTH)
        imageAspect = ImageAspectFlagBits::DEPTH_BIT;
    else if (formatAttribute.baseFormat == BaseFormat::STENCIL)
        imageAspect = ImageAspectFlagBits::STENCIL_BIT;
    else if (formatAttribute.baseFormat == BaseFormat::DEPTH_STENCIL)
        imageAspect = ImageAspectFlagBits::DEPTH_BIT | ImageAspectFlagBits::STENCIL_BIT;
    else
        imageAspect = ImageAspectFlagBits::COLOR_BIT;

    ImageMemoryBarrier barrier{AccessFlagBits::MEMORY_WRITE_BIT,
                               AccessFlagBits::MEMORY_READ_BIT,
                               info.srcImageLayout,
                               ImageLayout::TRANSFER_DST_OPTIMAL,
                               ST_QUEUE_FAMILY_IGNORED,
                               ST_QUEUE_FAMILY_IGNORED,
                               info.pImage,
                               {imageAspect, 0, pImageCreateInfo->mipLevels, 0, pImageCreateInfo->arrayLayers}};

    PipelineBarrier(Shit::PipelineBarrierInfo{
        PipelineStageFlagBits::ALL_COMMANDS_BIT, PipelineStageFlagBits::TRANSFER_BIT, {}, 0, 0, 0, 0, 1, &barrier});

    ImageBlit blitRegion{
        // src
        {
            imageAspect,
            0,                             // miplevel
            0,                             // base array layer
            pImageCreateInfo->arrayLayers  // layer count
        },
        {{}, {}},  // offset
        // dst
        {imageAspect, 1, 0, pImageCreateInfo->arrayLayers},
        {{}, {}}  // offset
    };

    barrier.subresourceRange.levelCount = 1;
    auto mipExtent = pImageCreateInfo->extent;
    for (uint32_t i = 1; i < pImageCreateInfo->mipLevels; ++i) {
        barrier.oldImageLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
        barrier.newImageLayout = ImageLayout::TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = AccessFlagBits::TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = AccessFlagBits::TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;
        PipelineBarrier(Shit::PipelineBarrierInfo{
            PipelineStageFlagBits::TRANSFER_BIT, PipelineStageFlagBits::TRANSFER_BIT, {}, 0, 0, 0, 0, 1, &barrier});
        blitRegion.srcSubresource.mipLevel = i - 1;
        blitRegion.srcOffsets[1] = {static_cast<int32_t>(mipExtent.width), static_cast<int32_t>(mipExtent.height),
                                    static_cast<int32_t>(mipExtent.depth)};

        mipExtent.width = (std::max)(mipExtent.width >> 1, 1u);
        mipExtent.height = (std::max)(mipExtent.height >> 1, 1u);
        mipExtent.depth = (std::max)(mipExtent.depth >> 1, 1u);

        blitRegion.dstSubresource.mipLevel = i;
        blitRegion.dstOffsets[1] = {static_cast<int32_t>(mipExtent.width), static_cast<int32_t>(mipExtent.height),
                                    static_cast<int32_t>(mipExtent.depth)};
        BlitImage(BlitImageInfo{info.pImage, info.pImage, info.filter, 1, &blitRegion});
    }

    barrier.oldImageLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
    barrier.newImageLayout = info.dstImageLayout;
    barrier.srcAccessMask = AccessFlagBits::TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = AccessFlagBits::MEMORY_READ_BIT;
    barrier.subresourceRange.baseMipLevel = pImageCreateInfo->mipLevels - 1;
    PipelineBarrier(Shit::PipelineBarrierInfo{
        PipelineStageFlagBits::TRANSFER_BIT, PipelineStageFlagBits::ALL_COMMANDS_BIT, {}, 0, 0, 0, 0, 1, &barrier});

    if (pImageCreateInfo->mipLevels > 1) {
        barrier.oldImageLayout = ImageLayout::TRANSFER_SRC_OPTIMAL;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = pImageCreateInfo->mipLevels - 1;
        PipelineBarrier(Shit::PipelineBarrierInfo{
            PipelineStageFlagBits::TRANSFER_BIT, PipelineStageFlagBits::ALL_COMMANDS_BIT, {}, 0, 0, 0, 0, 1, &barrier});
    }
}
}  // namespace Shit
