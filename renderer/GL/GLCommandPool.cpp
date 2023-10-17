/**
 * @file GLCommandPool.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLCommandPool.hpp"
namespace Shit {
void GLCommandPool::CreateCommandBuffersImpl(const CommandBufferCreateInfo &createInfo) {
    for (uint32_t i = 0; i < createInfo.count; ++i)
        mCommandBuffers.emplace_back(new GLCommandBuffer(mpStateManager, this, createInfo.level));
}
}  // namespace Shit