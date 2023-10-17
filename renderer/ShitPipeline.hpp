/**
 * @file ShitPipeline.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"

namespace Shit {

class PipelineLayout {
protected:
    PipelineLayoutCreateInfo mCreateInfo;
    PipelineLayout(const PipelineLayoutCreateInfo &createInfo) {
        mCreateInfo.setLayoutCount = createInfo.setLayoutCount;
        if (mCreateInfo.setLayoutCount) {
            mCreateInfo.pSetLayouts = new const DescriptorSetLayout *[mCreateInfo.setLayoutCount];
            memcpy((void *)mCreateInfo.pSetLayouts, createInfo.pSetLayouts,
                   sizeof(std::ptrdiff_t) * mCreateInfo.setLayoutCount);
        }

        mCreateInfo.pushConstantRangeCount = createInfo.pushConstantRangeCount;
        if (mCreateInfo.pushConstantRangeCount) {
            mCreateInfo.pPushConstantRanges = new PushConstantRange[mCreateInfo.pushConstantRangeCount];
            memcpy((void *)mCreateInfo.pPushConstantRanges, createInfo.pPushConstantRanges,
                   sizeof(PushConstantRange) * mCreateInfo.pushConstantRangeCount);
        }
    }

public:
    virtual ~PipelineLayout() {
        if (mCreateInfo.setLayoutCount) delete[] mCreateInfo.pSetLayouts;
        if (mCreateInfo.pushConstantRangeCount) delete[] mCreateInfo.pPushConstantRanges;
    }
    constexpr const PipelineLayoutCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};

class Pipeline {
    /**
     * @brief destroy gpu object
     *
     */
    virtual void Destroy() = 0;

public:
    virtual ~Pipeline() {}
    virtual void Initialize() = 0;

    virtual PipelineLayout const *GetPipelineLayout() const = 0;
    virtual PipelineBindPoint GetBindPoint() const = 0;

    void Reinitialize() {
        Destroy();
        Initialize();
    }
};

class GraphicsPipeline : public virtual Pipeline {
protected:
    GraphicsPipelineCreateInfo mCreateInfo;
    GraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo);

public:
    virtual ~GraphicsPipeline();
    const GraphicsPipelineCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};

class ComputePipeline : public virtual Pipeline {
protected:
    ComputePipelineCreateInfo mCreateInfo;
    ComputePipeline(const ComputePipelineCreateInfo &createInfo);

public:
    virtual ~ComputePipeline();
    const ComputePipelineCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};
}  // namespace Shit
