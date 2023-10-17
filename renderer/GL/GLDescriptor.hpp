/**
 * @file GLDescriptor.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitDescriptor.hpp>

#include "GLBufferView.hpp"
#include "GLPrerequisites.hpp"

namespace Shit {
class GLDescriptorSetLayout final : public DescriptorSetLayout {
public:
    GLDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) : DescriptorSetLayout(createInfo) {}
};

/**
 * @brief TODO: dynamic uniform and storage buffer
 *
 */
class GLDescriptorSet final : public DescriptorSet {
public:
    struct DescriptorInfo {
        // uint32_t binding;
        DescriptorType type;
        std::variant<DescriptorBufferInfo, DescriptorImageInfo, const BufferView *> info;
    };

    GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout);

    void Set(DescriptorType type, uint32_t binding, uint32_t count, BufferView const *const *values);
    void Set(DescriptorType type, uint32_t binding, uint32_t count, DescriptorImageInfo const *values);
    void Set(DescriptorType type, uint32_t binding, uint32_t count, DescriptorBufferInfo const *values);

    void Bind(uint32_t const *pDynamicOffsets, uint32_t &index);

private:
    GLStateManager *mpStateManager;

    static constexpr size_t IMAGE_TEXTURE_DESCRIPTOR_MAX_COUNT = 32;
    static constexpr size_t TEXTURE_DESCRIPTOR_MAX_COUNT = 32;
    static constexpr size_t UNIFORM_BUFFER_DESCRIPTOR_MAX_COUNT = 16;
    static constexpr size_t STORAGE_BUFFER_DESCRIPTOR_MAX_COUNT = 16;

    // key is binding
    std::array<DescriptorInfo, IMAGE_TEXTURE_DESCRIPTOR_MAX_COUNT> mImageTextureDescriptorsInfo;
    std::array<DescriptorInfo, TEXTURE_DESCRIPTOR_MAX_COUNT> mTextureDescriptorsInfo;

    // indexed targets include  ATOMIC_COUNTER_BUFFER, SHADER_STORAGE_BUFFER,
    // TRANSFORM_FEEDBACK_BUFFER and UNIFORM_BUFFER.
    std::array<DescriptorInfo, UNIFORM_BUFFER_DESCRIPTOR_MAX_COUNT> mUniformBufferDescriptorsInfo;
    std::array<DescriptorInfo, STORAGE_BUFFER_DESCRIPTOR_MAX_COUNT> mStorageBufferDescriptorsInfo;
};
class GLDescriptorPool final : public DescriptorPool {
    GLStateManager *mpStateManager;

public:
    GLDescriptorPool(GLStateManager *pStateManager, const DescriptorPoolCreateInfo &createInfo)
        : DescriptorPool(createInfo), mpStateManager(pStateManager) {}

    void Allocate(const DescriptorSetAllocateInfo &allocateInfo, std::vector<DescriptorSet *> &descriptorSets) override;
    void Allocate(const DescriptorSetAllocateInfo &allocateInfo, DescriptorSet **ppDescriptorSets) override;
};
}  // namespace Shit
