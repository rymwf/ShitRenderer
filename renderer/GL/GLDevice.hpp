/**
 * @file GLDevice.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include <renderer/ShitDevice.hpp>

#ifdef _WIN32
typedef struct HDC__ *HDC;
typedef struct HGLRC__ *HGLRC;
#endif

namespace Shit {
/**
 * @brief in opengl, device is created based on windows, one window has its own
 * device
 *
 */
class GLDevice : public Device {
public:
    GLDevice(const DeviceCreateInfo &createInfo);

    ~GLDevice() override;

    RenderSystem *GetRenderSystem() const override;

    Result WaitIdle() const override;

    CommandPool *Create(const CommandPoolCreateInfo &createInfo) override;

    Shader *Create(const ShaderCreateInfo &createInfo) override;

    Pipeline *Create(const GraphicsPipelineCreateInfo &createInfo) override;

    Pipeline *Create(const ComputePipelineCreateInfo &createInfo) override;

    Buffer *Create(const BufferCreateInfo &createInfo, const void *pData) override;

    Buffer *Create(const BufferCreateInfo &createInfo, int val) override;

    BufferView *Create(const BufferViewCreateInfo &createInfo) override;

    Image *Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, const void *pData) override;

    Image *Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, int val) override;

    DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) override;

    ImageView *Create(const ImageViewCreateInfo &createInfo) override;
    PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) override;
    RenderPass *Create(const RenderPassCreateInfo &createInfo) override;
    Framebuffer *Create(const FramebufferCreateInfo &createInfo) override;
    Semaphore *Create(const SemaphoreCreateInfo &createInfo) override;
    Fence *Create(const FenceCreateInfo &createInfo) override;
    Sampler *Create(const SamplerCreateInfo &createInfo) override;
    DescriptorPool *Create(const DescriptorPoolCreateInfo &createInfo) override;

    void UpdateDescriptorSets(std::span<const WriteDescriptorSet> descriptorWrites,
                              std::span<const CopyDescriptorSet> descriptorCopies) override;
    void UpdateDescriptorSets(uint32_t descriptorWriteCount, const WriteDescriptorSet *descriptorWrites,
                              uint32_t descriptorCopyCount, const CopyDescriptorSet *descriptorCopies) override;

    void GetSupportedShaderSourceLanguages(std::vector<ShadingLanguage> &shadingLanguage) const override;
};
}  // namespace Shit