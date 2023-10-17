/**
 * @file ShitDescriptor.hpp
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

class DescriptorSetLayout {
protected:
    DescriptorSetLayoutCreateInfo mCreateInfo{};
    DescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) {
        if ((mCreateInfo.descriptorSetLayoutBindingCount = createInfo.descriptorSetLayoutBindingCount) != 0) {
            mCreateInfo.pDescriptorSetLayoutBindings =
                new DescriptorSetLayoutBinding[mCreateInfo.descriptorSetLayoutBindingCount];
            memcpy((void *)mCreateInfo.pDescriptorSetLayoutBindings, createInfo.pDescriptorSetLayoutBindings,
                   sizeof(DescriptorSetLayoutBinding) * mCreateInfo.descriptorSetLayoutBindingCount);
        }
        for (uint32_t i = 0; i < mCreateInfo.descriptorSetLayoutBindingCount; ++i) {
            auto &&e = const_cast<DescriptorSetLayoutBinding *>(&mCreateInfo.pDescriptorSetLayoutBindings[i]);
            if (e->immutableSamplerCount) {
                e->pImmutableSamplers = new Sampler const *[e->immutableSamplerCount];
                memcpy((void *)e->pImmutableSamplers, createInfo.pDescriptorSetLayoutBindings[i].pImmutableSamplers,
                       sizeof(std::ptrdiff_t) * e->immutableSamplerCount);
            }
        }
    }

public:
    virtual ~DescriptorSetLayout() {
        for (uint32_t i = 0; i < mCreateInfo.descriptorSetLayoutBindingCount; ++i)
            delete[] mCreateInfo.pDescriptorSetLayoutBindings[i].pImmutableSamplers;
        delete[] mCreateInfo.pDescriptorSetLayoutBindings;
    }
    constexpr const DescriptorSetLayoutCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    DescriptorSetLayoutBinding const *GetSetLayoutBinding(uint32_t binding) const {
        for (uint32_t i = 0; i < mCreateInfo.descriptorSetLayoutBindingCount; ++i) {
            if (mCreateInfo.pDescriptorSetLayoutBindings[i].binding == binding)
                return &mCreateInfo.pDescriptorSetLayoutBindings[i];
        }
        return nullptr;
    }
};

class DescriptorSet {
protected:
    const DescriptorSetLayout *mpSetLayout;
    DescriptorSet(const DescriptorSetLayout *pSetLayout) : mpSetLayout(pSetLayout) {}

public:
    virtual ~DescriptorSet() {}
    constexpr const DescriptorSetLayout *GetSetLayoutPtr() const { return mpSetLayout; }
};

class DescriptorPool {
protected:
    DescriptorPoolCreateInfo mCreateInfo;
    std::unordered_set<DescriptorSet *> mDescriptorSets;

public:
    DescriptorPool(const DescriptorPoolCreateInfo &createInfo) : mCreateInfo(createInfo) {
        mCreateInfo.poolSizes = new DescriptorPoolSize[mCreateInfo.poolSizeCount];
        memcpy((void *)mCreateInfo.poolSizes, createInfo.poolSizes,
               sizeof(DescriptorPoolSize) * mCreateInfo.poolSizeCount);
    }
    virtual ~DescriptorPool() { delete[] mCreateInfo.poolSizes; }

    /**
     * @brief in opengl, only the first descriptor set works
     *
     * @param allocateInfo
     * @param descriptorSets
     */
    virtual void Allocate(const DescriptorSetAllocateInfo &allocateInfo,
                          std::vector<DescriptorSet *> &descriptorSets) = 0;

    /**
     * @brief
     *
     * @param allocateInfo
     * @param count
     * @param ppDescriptorSets
     */
    virtual void Allocate(const DescriptorSetAllocateInfo &allocateInfo, DescriptorSet **ppDescriptorSets) = 0;
    constexpr const DescriptorPoolCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    void Free(size_t count, DescriptorSet const *const *ppDescriptorSets) {
        while (count-- > 0) mDescriptorSets.erase(const_cast<DescriptorSet *>(ppDescriptorSets[count]));
    }
};
}  // namespace Shit