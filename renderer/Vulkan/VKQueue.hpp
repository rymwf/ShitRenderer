/**
 * @file VKQueue.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitQueue.hpp>

#include "VKDeviceObject.hpp"
namespace Shit {
class VKQueue final : public Queue, public VKDeviceObject {
    VkQueue mHandle;

public:
    VKQueue(VKDevice *device, uint32_t familyIndex, uint32_t index, float priority)
        : VKDeviceObject(device), Queue(familyIndex, index, priority) {
        vkGetDeviceQueue(mpDevice->GetHandle(), familyIndex, index, &mHandle);
    }
    constexpr VkQueue GetHandle() const { return mHandle; }
    void Submit(std::span<const SubmitInfo> submitInfos, Fence *pFence) override;

    Result Present(const PresentInfo &presentInfo) override;
    Result Present(const PresentInfo2 &presentInfo) override;

    void WaitIdle() override;
};
}  // namespace Shit
