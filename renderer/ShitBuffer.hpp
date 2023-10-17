/**
 * @file ShitBuffer.hpp
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
class Buffer {
protected:
    BufferCreateInfo mCreateInfo;
    uint64_t mMemorySize{};

public:
    Buffer(const BufferCreateInfo &createInfo) : mCreateInfo(createInfo), mMemorySize(createInfo.size) {}
    constexpr const BufferCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    /**
     * @brief will clear buffer data
     *
     * @param size
     */
    virtual void Resize(uint64_t size) = 0;
    constexpr uint64_t GetMemorySize() const { return mMemorySize; }
    virtual ~Buffer() {}
    virtual void MapMemory(uint64_t offset, uint64_t size, void **ppData) const = 0;
    virtual void UnMapMemory() const = 0;
    virtual void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const = 0;
    virtual void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const = 0;

    virtual void UpdateSubData(uint64_t offset, uint64_t size, void const *pData) const = 0;
    virtual void UpdateSubData(uint64_t offset, uint64_t size, int val) const = 0;
};
}  // namespace Shit
