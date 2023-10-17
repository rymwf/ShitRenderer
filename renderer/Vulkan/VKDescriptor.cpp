/**
 * @file VKDescriptor.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKDescriptor.hpp"

#include "VKSampler.hpp"
namespace Shit {
VKDescriptorSetLayout::VKDescriptorSetLayout(VKDevice *pDevice, const DescriptorSetLayoutCreateInfo &createInfo)
    : DescriptorSetLayout(createInfo), VKDeviceObject(pDevice) {
    std::vector<VkDescriptorSetLayoutBinding> bindings(mCreateInfo.descriptorSetLayoutBindingCount);
    std::vector<std::vector<VkSampler>> samplers(mCreateInfo.descriptorSetLayoutBindingCount);
    for (uint32_t i = 0, j; i < mCreateInfo.descriptorSetLayoutBindingCount; ++i) {
        auto &&e = mCreateInfo.pDescriptorSetLayoutBindings[i];
        samplers[i].resize(e.immutableSamplerCount);
        for (j = 0; j < e.immutableSamplerCount; ++j)
            samplers[i][j] = (static_cast<const VKSampler *>(e.pImmutableSamplers[j])->GetHandle());

        bindings[i] =
            VkDescriptorSetLayoutBinding{e.binding, Map(e.descriptorType), e.descriptorCount,
                                         static_cast<VkShaderStageFlags>(Map(e.stageFlags)), samplers[i].data()};
    }
    VkDescriptorSetLayoutCreateInfo info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
                                         static_cast<uint32_t>(bindings.size()), bindings.data()};

    CHECK_VK_RESULT(vkCreateDescriptorSetLayout(mpDevice->GetHandle(), &info, nullptr, &mHandle))
}
VKDescriptorSet::VKDescriptorSet(VKDevice *device, VkDescriptorPool pool, const DescriptorSetLayout *pSetLayout)
    : DescriptorSet(pSetLayout), VKDeviceObject(device), mPool(pool) {
    auto a = static_cast<const VKDescriptorSetLayout *>(pSetLayout)->GetHandle();
    VkDescriptorSetAllocateInfo allocInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, nullptr, mPool, 1u, &a};
    CHECK_VK_RESULT(vkAllocateDescriptorSets(mpDevice->GetHandle(), &allocInfo, &mHandle));
}

VKDescriptorPool::VKDescriptorPool(VKDevice *device, const DescriptorPoolCreateInfo &createInfo)
    : DescriptorPool(createInfo), VKDeviceObject(device) {
    std::vector<VkDescriptorPoolSize> poolSizes;
    poolSizes.reserve(mCreateInfo.poolSizeCount);
    for (uint32_t i = 0; i < mCreateInfo.poolSizeCount; ++i) {
        auto &&e = mCreateInfo.poolSizes[i];
        poolSizes.emplace_back(Map(e.type), e.count);
    }

    VkDescriptorPoolCreateInfo info{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, nullptr,         0, createInfo.maxSets,
        static_cast<uint32_t>(poolSizes.size()),       poolSizes.data()};

    CHECK_VK_RESULT(vkCreateDescriptorPool(mpDevice->GetHandle(), &info, nullptr, &mHandle));
}
void VKDescriptorPool::Allocate(const DescriptorSetAllocateInfo &allocateInfo,
                                std::vector<DescriptorSet *> &descriptorSets) {
    descriptorSets.resize(allocateInfo.setLayoutCount);
    Allocate(allocateInfo, descriptorSets.data());
}
void VKDescriptorPool::Allocate(const DescriptorSetAllocateInfo &allocateInfo, DescriptorSet **ppDescriptorSets) {
    for (size_t i = 0; i < allocateInfo.setLayoutCount; ++i) {
        ppDescriptorSets[i] =
            *mDescriptorSets.emplace(new VKDescriptorSet(mpDevice, mHandle, allocateInfo.setLayouts[i])).first;
    }
}
}  // namespace Shit