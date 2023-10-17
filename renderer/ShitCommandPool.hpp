/**
 * @file ShitCommandPool.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitCommandBuffer.hpp"
#include "ShitRendererPrerequisites.hpp"

namespace Shit {
class CommandPool {
protected:
    CommandPoolCreateInfo mCreateInfo;
    std::vector<CommandBuffer *> mCommandBuffers;

    CommandPool(const CommandPoolCreateInfo &createInfo) : mCreateInfo(createInfo) {}

    virtual void CreateCommandBuffersImpl(const CommandBufferCreateInfo &createInfo) = 0;

public:
    virtual ~CommandPool() {
        for (auto e : mCommandBuffers) delete e;
    }

    void CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, CommandBuffer **commandBuffers) {
        auto offset = mCommandBuffers.size();
        CreateCommandBuffersImpl(createInfo);
        memcpy(commandBuffers, &mCommandBuffers[offset], sizeof(ptrdiff_t) * createInfo.count);
    }

    void DestroyCommandBuffer(CommandBuffer *pCommandBuffer) {
        for (auto it = mCommandBuffers.begin(); it != mCommandBuffers.end(); ++it) {
            if (*it == pCommandBuffer) {
                delete *it;
                mCommandBuffers.erase(it);
                break;
            }
        }
    }
    constexpr const CommandPoolCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};
}  // namespace Shit