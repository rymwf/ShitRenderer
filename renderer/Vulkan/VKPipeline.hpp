/**
 * @file VKPipeline.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitPipeline.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKPipelineLayout final : public PipelineLayout, public VKDeviceObject {
    VkPipelineLayout mHandle{};

public:
    VKPipelineLayout(VKDevice *device, const PipelineLayoutCreateInfo &createInfo);
    ~VKPipelineLayout() override { vkDestroyPipelineLayout(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkPipelineLayout GetHandle() const { return mHandle; }
};

class VKPipeline : public virtual Pipeline, public VKDeviceObject {
protected:
    VkPipeline mHandle;

    void Destroy() override {
        vkDestroyPipeline(mpDevice->GetHandle(), mHandle, nullptr);
        mHandle = nullptr;
    }

    static void ConvertPipelineShaderStageInfo(uint32_t stageCount, PipelineShaderStageCreateInfo const *stages,
                                               std::vector<VkPipelineShaderStageCreateInfo> &dst);

public:
    VKPipeline(VKDevice *device) : VKDeviceObject(device) {}
    ~VKPipeline() override { Destroy(); }
    VkPipeline GetHandle() const { return mHandle; }
};

class VKGraphicsPipeline final : public GraphicsPipeline, public VKPipeline {
    void Destroy() override { VKPipeline::Destroy(); }
    void Initialize() override;

public:
    VKGraphicsPipeline(VKDevice *device, const GraphicsPipelineCreateInfo &createInfo);

    PipelineLayout const *GetPipelineLayout() const override { return mCreateInfo.pLayout; }
    PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::GRAPHICS; }
};
class VKComputePipeline final : public ComputePipeline, public VKPipeline {
    void Destroy() override { VKPipeline::Destroy(); }
    void Initialize() override;

public:
    VKComputePipeline(VKDevice *device, const ComputePipelineCreateInfo &createInfo);
    PipelineLayout const *GetPipelineLayout() const override { return mCreateInfo.pLayout; }
    PipelineBindPoint GetBindPoint() const override { return PipelineBindPoint::COMPUTE; }
};
}  // namespace Shit
