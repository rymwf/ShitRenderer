/**
 * @file VKDescriptor.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitDescriptor.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKDescriptorSetLayout final : public DescriptorSetLayout, public VKDeviceObject {
    VkDescriptorSetLayout mHandle;

public:
    VKDescriptorSetLayout(VKDevice *pDevice, const DescriptorSetLayoutCreateInfo &createInfo);
    ~VKDescriptorSetLayout() override { vkDestroyDescriptorSetLayout(mpDevice->GetHandle(), mHandle, nullptr); }

    constexpr VkDescriptorSetLayout GetHandle() const { return mHandle; }
};
class VKDescriptorSet final : public DescriptorSet, public VKDeviceObject {
    VkDescriptorSet mHandle;
    VkDescriptorPool mPool;

public:
    VKDescriptorSet(VKDevice *device, VkDescriptorPool pool, const DescriptorSetLayout *pSetlayout);
    ~VKDescriptorSet() override { vkFreeDescriptorSets(mpDevice->GetHandle(), mPool, 1, &mHandle); }
    constexpr VkDescriptorSet GetHandle() const { return mHandle; }
};
class VKDescriptorPool final : public DescriptorPool, public VKDeviceObject {
    VkDescriptorPool mHandle;

public:
    VKDescriptorPool(VKDevice *device, const DescriptorPoolCreateInfo &createInfo);
    ~VKDescriptorPool() override {
        mDescriptorSets.clear();
        vkDestroyDescriptorPool(mpDevice->GetHandle(), mHandle, nullptr);
    }
    void Allocate(const DescriptorSetAllocateInfo &allocateInfo, std::vector<DescriptorSet *> &descriptorSets) override;
    void Allocate(const DescriptorSetAllocateInfo &allocateInfo, DescriptorSet **ppDescriptorSets) override;
};
}  // namespace Shit
