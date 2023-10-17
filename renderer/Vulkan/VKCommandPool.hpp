/**
 * @file VKCommandPool.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitCommandPool.hpp>

#include "VKCommandBuffer.hpp"
#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKCommandPool final : public CommandPool, public VKDeviceObject {
    VkCommandPool mHandle;

    void CreateCommandBuffersImpl(const CommandBufferCreateInfo &createInfo) override {
        for (uint32_t i = 0; i < createInfo.count; ++i)
            mCommandBuffers.emplace_back(new VKCommandBuffer(mpDevice, this, createInfo.level));
    }

public:
    VKCommandPool(VKDevice *device, const CommandPoolCreateInfo &createInfo)
        : CommandPool(createInfo), VKDeviceObject(device) {
        VkCommandPoolCreateInfo commandPoolCreateInfo{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
                                                      Map(mCreateInfo.flags), mCreateInfo.queueFamilyIndex};
        CHECK_VK_RESULT(vkCreateCommandPool(mpDevice->GetHandle(), &commandPoolCreateInfo, nullptr, &mHandle))
    }
    constexpr VkCommandPool GetHandle() const { return mHandle; }
    ~VKCommandPool() override { vkDestroyCommandPool(mpDevice->GetHandle(), mHandle, nullptr); }
};
}  // namespace Shit
