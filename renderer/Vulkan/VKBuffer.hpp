/**
 * @file VKBuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitBuffer.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"

namespace Shit {
class VKBuffer final : public Buffer, public VKDeviceObject {
    VkBuffer mHandle;
    VkDeviceMemory mMemory;
    VkPhysicalDevice mPhysicalDevice;

public:
    VKBuffer(VKDevice *pDevice, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo);
    ~VKBuffer() override {
        vkFreeMemory(mpDevice->GetHandle(), mMemory, nullptr);
        vkDestroyBuffer(mpDevice->GetHandle(), mHandle, nullptr);
    }
    constexpr VkBuffer GetHandle() const { return mHandle; }
    constexpr VkDeviceMemory GetMemory() const { return mMemory; }

    void Resize(uint64_t size) override;
    void MapMemory(uint64_t offset, uint64_t size, void **ppData) const override;
    void UnMapMemory() const override;
    void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    /**
     * @brief ��֤�����Ѿ�д����gpu��ֻ�е��ڴ治��COHERENTʱ�ſ�ʹ��
     *
     * @param offset
     * @param size
     */
    void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const override;

    void UpdateSubData(uint64_t offset, uint64_t size, void const *pData) const override;
    void UpdateSubData(uint64_t offset, uint64_t size, int val) const override;
};
}  // namespace Shit
