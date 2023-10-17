/**
 * @file VKFramebuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitFramebuffer.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKFramebuffer final : public Framebuffer, public VKDeviceObject {
    VkFramebuffer mHandle;

public:
    VKFramebuffer(VKDevice *device, const FramebufferCreateInfo &createInfo);
    ~VKFramebuffer() override { vkDestroyFramebuffer(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkFramebuffer GetHandle() const { return mHandle; }
};
}  // namespace Shit
