/**
 * @file VKFence.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitFence.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKFence final : public Fence, public VKDeviceObject {
    VkFence mHandle;

public:
    VKFence(VKDevice *device, const FenceCreateInfo &createInfo) : Fence(createInfo), VKDeviceObject(device) {
        VkFenceCreateInfo info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, Map(createInfo.flags)};
        CHECK_VK_RESULT(vkCreateFence(mpDevice->GetHandle(), &info, nullptr, &mHandle));
    }
    ~VKFence() override { vkDestroyFence(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkFence GetHandle() const { return mHandle; }
    void Reset() const override { vkResetFences(mpDevice->GetHandle(), 1, &mHandle); }
    Result WaitFor(uint64_t timeout) const override {
        switch (vkWaitForFences(mpDevice->GetHandle(), 1, &mHandle, VK_TRUE, timeout)) {
            case VK_SUCCESS:
                return Result::SUCCESS;
            case VK_TIMEOUT:
                return Result::TIMEOUT;
            default:
                return Result::SHIT_ERROR;
        }
    }
};
}  // namespace Shit
