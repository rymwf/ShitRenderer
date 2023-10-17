/**
 * @file GLCommandPool.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitCommandPool.hpp>

#include "GLCommandBuffer.hpp"
#include "GLPrerequisites.hpp"

namespace Shit {
class GLCommandPool final : public CommandPool {
    GLStateManager *mpStateManager;

    void CreateCommandBuffersImpl(const CommandBufferCreateInfo &createInfo) override;

public:
    GLCommandPool(GLStateManager *pStateManger, const CommandPoolCreateInfo &createInfo)
        : CommandPool(createInfo), mpStateManager(pStateManger) {}
    ~GLCommandPool() override {}
};

}  // namespace Shit
