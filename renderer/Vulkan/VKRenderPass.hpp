/**
 * @file VKRenderPass.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitRenderPass.hpp>

#include "VKDeviceObject.hpp"
namespace Shit {
class VKRenderPass final : public RenderPass, public VKDeviceObject {
    VkRenderPass mHandle;

public:
    VKRenderPass(VKDevice *device, const RenderPassCreateInfo &createInfo);
    ~VKRenderPass() override { vkDestroyRenderPass(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkRenderPass GetHandle() const { return mHandle; }
};
}  // namespace Shit
