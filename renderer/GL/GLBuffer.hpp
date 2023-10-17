/**
 * @file GLBuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitBuffer.hpp>

#include "GLPrerequisites.hpp"

namespace Shit {
/**
 * @brief when using immutable format, buffer cannot be updated by glBufferData
 * glGetBufferSubData, glBufferSubData will copy data before function returns,
 * thus it will slow than map
 *
 */
class GLBuffer final : public Buffer {
    GLuint mHandle;
    GLStateManager *mpStateManager;

    void CreateBuffer(void const *pData);
    void CreateBuffer(int val);

public:
    GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, const void *pData);
    GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, int val);
    ~GLBuffer() override;

    constexpr GLuint GetHandle() const { return mHandle; }
    void Resize(uint64_t size) override;
    void MapMemory(uint64_t offset, uint64_t size, void **ppData) const override;
    void UnMapMemory() const override;
    void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const override;

    void UpdateSubData(uint64_t offset, uint64_t size, void const *pData) const override;
    void UpdateSubData(uint64_t offset, uint64_t size, int val) const override;
};
}  // namespace Shit
