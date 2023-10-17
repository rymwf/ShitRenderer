/**
 * @file VKBufferView.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitBufferView.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"

namespace Shit {
class VKBufferView final : public BufferView, public VKDeviceObject {
    VkBufferView mHandle;

public:
    VKBufferView(VKDevice* pDevice, const BufferViewCreateInfo& createInfo);
    ~VKBufferView() override { vkDestroyBufferView(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkBufferView GetHandle() const { return mHandle; }
};
}  // namespace Shit
