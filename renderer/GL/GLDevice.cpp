/**
 * @file GLDevice.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLDevice.hpp"

#include "GLBuffer.hpp"
#include "GLCommandBuffer.hpp"
#include "GLCommandPool.hpp"
#include "GLDescriptor.hpp"
#include "GLFence.hpp"
#include "GLFramebuffer.hpp"
#include "GLImage.hpp"
#include "GLPipeline.hpp"
#include "GLQueue.hpp"
#include "GLRenderPass.hpp"
#include "GLRenderSystem.hpp"
#include "GLSampler.hpp"
#include "GLSemaphore.hpp"
#include "GLShader.hpp"
#include "GLState.hpp"
#include "GLSurface.hpp"
#include "GLSwapchain.hpp"

namespace Shit {
GLDevice::GLDevice(const DeviceCreateInfo &createInfo) : Device(createInfo) {
    mQueues.resize(1);
    // TODO:: queue prority
    mQueues[0].emplace_back(
        std::make_unique<GLQueue>(this, g_RenderSystem->GetMainSurface()->GetStateManager(), 0u, 0u, 0.f));

    Init();
}
GLDevice::~GLDevice() {
    mCommandPools.clear();
    mPipelines.clear();
    mShaders.clear();
    mBuffers.clear();
    mImages.clear();
    mImageViews.clear();
    mDescriptorSetLayouts.clear();
    mPipelineLayouts.clear();
    mRenderPasses.clear();
    mFramebuffers.clear();
    mSemaphores.clear();
    mFences.clear();
    mSamplers.clear();
    mDescriptorPools.clear();
}
RenderSystem *GLDevice::GetRenderSystem() const { return g_RenderSystem; }
Result GLDevice::WaitIdle() const {
    glFinish();
    return Result::SUCCESS;
}
CommandPool *GLDevice::Create(const CommandPoolCreateInfo &createInfo) {
    return mCommandPools
        .emplace_back(std::make_unique<GLCommandPool>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Shader *GLDevice::Create(const ShaderCreateInfo &createInfo) {
    return mShaders.emplace_back(std::make_unique<GLShader>(createInfo)).get();
}
Pipeline *GLDevice::Create(const GraphicsPipelineCreateInfo &createInfo) {
    return mPipelines
        .emplace_back(
            std::make_unique<GLGraphicsPipeline>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Pipeline *GLDevice::Create(const ComputePipelineCreateInfo &createInfo) {
    return mPipelines
        .emplace_back(
            std::make_unique<GLComputePipeline>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Buffer *GLDevice::Create(const BufferCreateInfo &createInfo, const void *pData) {
    return mBuffers
        .emplace_back(
            std::make_unique<GLBuffer>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo, pData))
        .get();
}
Buffer *GLDevice::Create(const BufferCreateInfo &createInfo, int val) {
    return mBuffers
        .emplace_back(std::make_unique<GLBuffer>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo, val))
        .get();
}
BufferView *GLDevice::Create(const BufferViewCreateInfo &createInfo) {
    return mBufferViews
        .emplace_back(std::make_unique<GLBufferView>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Image *GLDevice::Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits, const void *pData) {
    return mImages
        .emplace_back(std::make_unique<GLImage>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo, pData))
        .get();
}
Image *GLDevice::Create(const ImageCreateInfo &createInfo, ImageAspectFlagBits, int val) {
    return mImages
        .emplace_back(std::make_unique<GLImage>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo, val))
        .get();
}
DescriptorSetLayout *GLDevice::Create(const DescriptorSetLayoutCreateInfo &createInfo) {
    return mDescriptorSetLayouts.emplace_back(std::make_unique<GLDescriptorSetLayout>(createInfo)).get();
}
ImageView *GLDevice::Create(const ImageViewCreateInfo &createInfo) {
    return mImageViews
        .emplace_back(std::make_unique<GLImageView>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
PipelineLayout *GLDevice::Create(const PipelineLayoutCreateInfo &createInfo) {
    return mPipelineLayouts
        .emplace_back(
            std::make_unique<GLPipelineLayout>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
RenderPass *GLDevice::Create(const RenderPassCreateInfo &createInfo) {
    return mRenderPasses.emplace_back(std::make_unique<GLRenderPass>(createInfo)).get();
}
Framebuffer *GLDevice::Create(const FramebufferCreateInfo &createInfo) {
    return mFramebuffers
        .emplace_back(std::make_unique<GLFramebuffer>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Semaphore *GLDevice::Create(const SemaphoreCreateInfo &createInfo) {
    return mSemaphores
        .emplace_back(std::make_unique<GLSemaphore>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Fence *GLDevice::Create(const FenceCreateInfo &createInfo) {
    return mFences
        .emplace_back(std::make_unique<GLFence>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
Sampler *GLDevice::Create(const SamplerCreateInfo &createInfo) {
    return mSamplers
        .emplace_back(std::make_unique<GLSampler>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
DescriptorPool *GLDevice::Create(const DescriptorPoolCreateInfo &createInfo) {
    return mDescriptorPools
        .emplace_back(
            std::make_unique<GLDescriptorPool>(g_RenderSystem->GetMainSurface()->GetStateManager(), createInfo))
        .get();
}
void GLDevice::UpdateDescriptorSets(std::span<const WriteDescriptorSet> descriptorWrites,
                                    std::span<const CopyDescriptorSet> descriptorCopies) {
    UpdateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(),
                         static_cast<uint32_t>(descriptorCopies.size()), descriptorCopies.data());
}
void GLDevice::UpdateDescriptorSets(uint32_t descriptorWriteCount, const WriteDescriptorSet *descriptorWrites,
                                    uint32_t descriptorCopyCount,
                                    ST_MAYBE_UNUSED const CopyDescriptorSet *descriptorCopies) {
    // for (auto &&write : descriptorWrites)
    for (uint32_t i = 0; i < descriptorWriteCount; ++i) {
        auto &&write = descriptorWrites[i];
        switch (write.descriptorType) {
            case DescriptorType::SAMPLER:  // sampler (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::COMBINED_IMAGE_SAMPLER:  // sampler2D
                ST_FALLTHROUGH;
            case DescriptorType::SAMPLED_IMAGE:  // texture2D (vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_IMAGE:  // image2D
                ST_FALLTHROUGH;
            case DescriptorType::INPUT_ATTACHMENT: {
                const_cast<GLDescriptorSet *>(static_cast<GLDescriptorSet const *>(write.pDstSet))
                    ->Set(write.descriptorType, write.dstBinding + write.dstArrayElement, write.descriptorCount,
                          &write.pImageInfo[0]);
                break;
            }
            case DescriptorType::UNIFORM_TEXEL_BUFFER:  // samplerbuffer	(access to buffer
                                                        // texture,can only be accessed with texelFetch
                                                        // function) ,textureBuffer(vulkan)
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_TEXEL_BUFFER:  // imagebuffer (access to
                                                        // buffer texture)
            {
                const_cast<GLDescriptorSet *>(static_cast<GLDescriptorSet const *>(write.pDstSet))
                    ->Set(write.descriptorType, write.dstBinding + write.dstArrayElement, write.descriptorCount,
                          &write.pTexelBufferView[0]);
                break;
            }
            case DescriptorType::UNIFORM_BUFFER:  // uniform block
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_BUFFER:  // buffer block
                ST_FALLTHROUGH;
            case DescriptorType::UNIFORM_BUFFER_DYNAMIC:
                ST_FALLTHROUGH;
            case DescriptorType::STORAGE_BUFFER_DYNAMIC: {
                const_cast<GLDescriptorSet *>(static_cast<GLDescriptorSet const *>(write.pDstSet))
                    ->Set(write.descriptorType, write.dstBinding + write.dstArrayElement, write.descriptorCount,
                          &write.pBufferInfo[0]);
                break;
            }
        }
    }
    if (descriptorCopyCount) {
        ST_LOG("descriptor copy not implemented yet");
    }
}
void GLDevice::GetSupportedShaderSourceLanguages(std::vector<ShadingLanguage> &shadingLanguage) const {
    shadingLanguage.emplace_back(ShadingLanguage::GLSL);
    std::vector<GLint> shaderBinaryFormats;
    GL::querySupportedShaderBinaryFormat(shaderBinaryFormats);
    auto it = std::find(shaderBinaryFormats.begin(), shaderBinaryFormats.end(), GL_SHADER_BINARY_FORMAT_SPIR_V);
    if (it == shaderBinaryFormats.end()) {
        ST_LOG("current gpu do not support binary shader format");
        return;
    } else
        shadingLanguage.emplace_back(ShadingLanguage::SPIRV);
}
}  // namespace Shit