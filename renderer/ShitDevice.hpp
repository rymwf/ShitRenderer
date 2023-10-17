/**
 * @file ShitDevice.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitBuffer.hpp"
#include "ShitBufferView.hpp"
#include "ShitCommandBuffer.hpp"
#include "ShitCommandPool.hpp"
#include "ShitDescriptor.hpp"
#include "ShitFence.hpp"
#include "ShitFramebuffer.hpp"
#include "ShitImage.hpp"
#include "ShitMemory.hpp"
#include "ShitPipeline.hpp"
#include "ShitQueue.hpp"
#include "ShitRenderPass.hpp"
#include "ShitRendererPrerequisites.hpp"
#include "ShitSampler.hpp"
#include "ShitSemaphore.hpp"
#include "ShitShader.hpp"
#include "ShitSurface.hpp"
#include "ShitSwapchain.hpp"

namespace Shit {

class Device {
protected:
    std::vector<std::vector<std::unique_ptr<Queue>>> mQueues;

    std::list<std::unique_ptr<CommandPool>> mCommandPools;
    std::list<std::unique_ptr<Pipeline>> mPipelines;
    std::list<std::unique_ptr<Shader>> mShaders;
    std::list<std::unique_ptr<Buffer>> mBuffers;
    std::list<std::unique_ptr<BufferView>> mBufferViews;
    std::list<std::unique_ptr<Image>> mImages;
    std::list<std::unique_ptr<ImageView>> mImageViews;
    std::list<std::unique_ptr<DescriptorSetLayout>> mDescriptorSetLayouts;
    std::list<std::unique_ptr<PipelineLayout>> mPipelineLayouts;
    std::list<std::unique_ptr<RenderPass>> mRenderPasses;
    std::list<std::unique_ptr<Framebuffer>> mFramebuffers;
    std::list<std::unique_ptr<Semaphore>> mSemaphores;
    std::list<std::unique_ptr<Fence>> mFences;
    std::list<std::unique_ptr<Sampler>> mSamplers;
    std::list<std::unique_ptr<DescriptorPool>> mDescriptorPools;

    DeviceCreateInfo mCreateInfo;

    CommandPool *mpOneTimeCommandPool;
    Queue *mpOneTimeCommandQueue;
    CommandBuffer *mpOneTimeCommandBuffer;

    void Init();

public:
    Device(const DeviceCreateInfo &createInfo);
    virtual ~Device() {}

    constexpr const DeviceCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }

    virtual Result WaitIdle() const = 0;

    virtual RenderSystem *GetRenderSystem() const = 0;

    void ExecuteOneTimeCommand(std::function<void(CommandBuffer *)> func);

    // virtual std::optional<QueueFamily> GetPresentQueueFamilyProperty(Window
    // const *) const
    // {
    // 	// for opengl
    // 	return std::optional<QueueFamily>{
    // 		{Shit::QueueFlagBits::GRAPHICS_BIT |
    // 			 Shit::QueueFlagBits::COMPUTE_BIT |
    // 			 Shit::QueueFlagBits::SPARSE_BINDING_BIT |
    // 			 Shit::QueueFlagBits::TRANSFER_BIT,
    // 		 0, 1}};
    // }
    // virtual void GetWindowPixelFormats(const Window *pWindow,
    // std::vector<WindowPixelFormat> &formats) const = 0;

    virtual Format GetSuitableImageFormat(std::span<const Format> candidates, ImageTiling,
                                          FormatFeatureFlagBits) const {
        return candidates[0];
    }
    virtual void GetSupportedShaderSourceLanguages(std::vector<ShadingLanguage> &shadingLanguage) const = 0;

    Queue *GetQueue(uint32_t familyIndex, uint32_t index) const;

    virtual Shader *Create(const ShaderCreateInfo &createInfo) = 0;
    virtual Pipeline *Create(const GraphicsPipelineCreateInfo &createInfo) = 0;
    virtual Pipeline *Create(const ComputePipelineCreateInfo &createInfo) = 0;

    virtual CommandPool *Create(const CommandPoolCreateInfo &createInfo) = 0;

    virtual Buffer *Create(const BufferCreateInfo &createInfo, const void *pData = nullptr) = 0;
    virtual Buffer *Create(const BufferCreateInfo &createInfo, int val) = 0;
    virtual BufferView *Create(const BufferViewCreateInfo &createInfo) = 0;

    virtual Image *Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask = {},
                          const void *pData = nullptr) = 0;

    virtual Image *Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, int val) = 0;

    virtual ImageView *Create(const ImageViewCreateInfo &createInfo) = 0;
    virtual DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) = 0;
    virtual PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) = 0;
    virtual RenderPass *Create(const RenderPassCreateInfo &createInfo) = 0;
    virtual Framebuffer *Create(const FramebufferCreateInfo &createInfo) = 0;
    virtual Semaphore *Create(const SemaphoreCreateInfo &createInfo) = 0;
    virtual Fence *Create(const FenceCreateInfo &createInfo) = 0;
    virtual Sampler *Create(const SamplerCreateInfo &createInfo) = 0;
    virtual DescriptorPool *Create(const DescriptorPoolCreateInfo &createInfo) = 0;

    virtual void UpdateDescriptorSets(std::span<const WriteDescriptorSet> descriptorWrites,
                                      std::span<const CopyDescriptorSet> descriptorCopies = {}) = 0;
    virtual void UpdateDescriptorSets(uint32_t descriptorWriteCount, const WriteDescriptorSet *descriptorWrites,
                                      uint32_t descriptorCopyCount, const CopyDescriptorSet *descriptorCopies) = 0;

    virtual void FlushMappedMemoryRanges(std::span<const MappedMemoryRange> ranges);
    virtual void FlushMappedMemoryRanges(uint32_t rangeCount, const MappedMemoryRange *ranges);
    virtual void InvalidateMappedMemoryRanges(std::span<const MappedMemoryRange> ranges);
    virtual void InvalidateMappedMemoryRanges(uint32_t rangeCount, const MappedMemoryRange *ranges);

    // void Destroy(const Swapchain *pSwapchain);
    void Destroy(const Shader *pShader);
    void Destroy(const Pipeline *pPipeline);
    void Destroy(const DescriptorSet *pDescriptorSet);
    void Destroy(const CommandPool *commandPool);
    void Destroy(const Buffer *pBuffer);
    void Destroy(const BufferView *pBufferView);
    void Destroy(const Image *pImage);
    void Destroy(const ImageView *pImageView);
    void Destroy(const DescriptorSetLayout *pSetLayout);
    void Destroy(const PipelineLayout *pLayout);
    void Destroy(const RenderPass *pRenderPass);
    void Destroy(const Semaphore *pSemaphore);
    void Destroy(const Fence *pFence);
    void Destroy(const Framebuffer *pFramebuffer);
    void Destroy(const DescriptorPool *pDescriptorPool);
    void Destroy(const Sampler *pSampler);
};
}  // namespace Shit
