/**
 * @file VKShader.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitShader.hpp>

#include "VKDeviceObject.hpp"

namespace Shit {

class VKShader final : public Shader, public VKDeviceObject {
    VkShaderModule mHandle;

    void Destroy() override { vkDestroyShaderModule(mpDevice->GetHandle(), mHandle, nullptr); }
    void Initialize() override {
        VkShaderModuleCreateInfo tempCreateInfo{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, nullptr, 0,
                                                mCreateInfo.size, reinterpret_cast<const uint32_t *>(mCreateInfo.code)};

        if (vkCreateShaderModule(mpDevice->GetHandle(), &tempCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
            ST_THROW("failed to create shadermodule");
    }

public:
    VKShader(VKDevice *device, const ShaderCreateInfo &createInfo) : Shader(createInfo), VKDeviceObject(device) {
        Initialize();
    }
    ~VKShader() override { Destroy(); }
    constexpr VkShaderModule GetHandle() const { return mHandle; }
};
}  // namespace Shit
