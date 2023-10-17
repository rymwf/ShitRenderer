/**
 * @file VKSampler.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSampler.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKSampler final : public Sampler, public VKDeviceObject {
    VkSampler mHandle;

public:
    VKSampler(VKDevice *device, const SamplerCreateInfo &createInfo);
    ~VKSampler() override { vkDestroySampler(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkSampler GetHandle() const { return mHandle; }
};
}  // namespace Shit
