/**
 * @file ShitDevice.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ShitDevice.hpp"

#include "ShitBuffer.hpp"
#include "ShitCommandBuffer.hpp"
#include "ShitCommandPool.hpp"
#include "ShitImage.hpp"
#include "ShitQueue.hpp"
#include "ShitRenderSystem.hpp"

namespace Shit {
Device::Device(const DeviceCreateInfo &createInfo) : mCreateInfo(createInfo) {}
void Device::Init() {
  // create default Commandpool
  auto oneTimeQueueFamilyProperty = GetRenderSystem()->GetQueueFamily(
      QueueFlagBits::GRAPHICS_BIT | QueueFlagBits::COMPUTE_BIT |
      QueueFlagBits::TRANSFER_BIT);

  mpOneTimeCommandPool = Create(
      CommandPoolCreateInfo{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
                            oneTimeQueueFamilyProperty->index});

  mpOneTimeCommandQueue = GetQueue(oneTimeQueueFamilyProperty->index, 0);
  mpOneTimeCommandPool->CreateCommandBuffers(
      CommandBufferCreateInfo{CommandBufferLevel::PRIMARY, 1},
      &mpOneTimeCommandBuffer);
}
Queue *Device::GetQueue(uint32_t familyIndex, uint32_t index) const {
  return mQueues.at(familyIndex).at(index).get();
}
void Device::Destroy(const Shader *pShader) {
  RemoveSmartPtrFromContainer(mShaders, pShader);
}
void Device::Destroy(const CommandPool *pCommandPool) {
  RemoveSmartPtrFromContainer(mCommandPools, pCommandPool);
}
void Device::Destroy(const Buffer *pBuffer) {
  RemoveSmartPtrFromContainer(mBuffers, pBuffer);
}
void Device::Destroy(const BufferView *pBufferView) {
  RemoveSmartPtrFromContainer(mBufferViews, pBufferView);
}
void Device::Destroy(const Image *pImage) {
  RemoveSmartPtrFromContainer(mImages, pImage);
}
void Device::Destroy(const ImageView *pImageView) {
  RemoveSmartPtrFromContainer(mImageViews, pImageView);
}
void Device::Destroy(const Semaphore *pSemaphore) {
  RemoveSmartPtrFromContainer(mSemaphores, pSemaphore);
}
void Device::Destroy(const Fence *pFence) {
  RemoveSmartPtrFromContainer(mFences, pFence);
}
void Device::Destroy(const DescriptorSetLayout *pSetLayout) {
  RemoveSmartPtrFromContainer(mDescriptorSetLayouts, pSetLayout);
}
void Device::Destroy(const PipelineLayout *pLayout) {
  RemoveSmartPtrFromContainer(mPipelineLayouts, pLayout);
}
void Device::Destroy(const RenderPass *pRenderPass) {
  RemoveSmartPtrFromContainer(mRenderPasses, pRenderPass);
}
void Device::Destroy(const Framebuffer *pFramebuffer) {
  RemoveSmartPtrFromContainer(mFramebuffers, pFramebuffer);
}
// void Device::Destroy(const Swapchain *pSwapchain)
// {
// 	RemoveSmartPtrFromContainer(mSwapchains, pSwapchain);
// }
void Device::Destroy(const Pipeline *pPipeline) {
  RemoveSmartPtrFromContainer(mPipelines, pPipeline);
}
void Device::Destroy(const DescriptorPool *pDescriptorPool) {
  RemoveSmartPtrFromContainer(mDescriptorPools, pDescriptorPool);
}
void Device::Destroy(const Sampler *pSampler) {
  RemoveSmartPtrFromContainer(mSamplers, pSampler);
}
void Device::FlushMappedMemoryRanges(
    std::span<const MappedMemoryRange> ranges) {
  FlushMappedMemoryRanges(static_cast<uint32_t>(ranges.size()), ranges.data());
}
void Device::FlushMappedMemoryRanges(uint32_t rangeCount,
                                     const MappedMemoryRange *ranges) {
  for (uint32_t i = 0; i < rangeCount; ++i) {
    auto &&e = ranges[i];
    if (auto p = *std::get_if<Buffer const *>(&e.memory))
      p->FlushMappedMemoryRange(e.offset, e.size);
    else if (auto p2 = *std::get_if<Image const *>(&e.memory))
      p2->FlushMappedMemoryRange(e.offset, e.size);
  }
}
void Device::InvalidateMappedMemoryRanges(
    std::span<const MappedMemoryRange> ranges) {
  InvalidateMappedMemoryRanges(static_cast<uint32_t>(ranges.size()),
                               ranges.data());
}
void Device::InvalidateMappedMemoryRanges(uint32_t rangeCount,
                                          const MappedMemoryRange *ranges) {
  for (uint32_t i = 0; i < rangeCount; ++i) {
    auto &&e = ranges[i];
    if (auto p = *std::get_if<Buffer const *>(&e.memory))
      p->InvalidateMappedMemoryRange(e.offset, e.size);
    else if (auto p2 = *std::get_if<Image const *>(&e.memory))
      p2->InvalidateMappedMemoryRange(e.offset, e.size);
  }
}
void Device::ExecuteOneTimeCommand(std::function<void(CommandBuffer *)> func) {
  static auto pFence = Create(Shit::FenceCreateInfo{});
  pFence->Reset();
  mpOneTimeCommandBuffer->Begin(
      CommandBufferBeginInfo{CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
  func(mpOneTimeCommandBuffer);
  mpOneTimeCommandBuffer->End();
  SubmitInfo submitInfo[]{{0, 0, 1, &mpOneTimeCommandBuffer}};
  mpOneTimeCommandQueue->Submit(submitInfo, pFence);
  pFence->WaitFor(UINT64_MAX);
}
}  // namespace Shit
