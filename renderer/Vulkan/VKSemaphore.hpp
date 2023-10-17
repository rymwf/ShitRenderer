/**
 * @file VKSemaphore.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSemaphore.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKSemaphore final : public Semaphore, public VKDeviceObject {
    VkSemaphore mHandle;

public:
    VKSemaphore(VKDevice *device, const SemaphoreCreateInfo &createInfo)
        : Semaphore(createInfo), VKDeviceObject(device) {
        VkSemaphoreCreateInfo info{
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        if (vkCreateSemaphore(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
            ST_THROW("failed to create semaphore");
    }
    ~VKSemaphore() override { vkDestroySemaphore(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkSemaphore GetHandle() const { return mHandle; }
};
}  // namespace Shit
