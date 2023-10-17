/**
 * @file VKSampler.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKSampler.hpp"
namespace Shit {
VKSampler::VKSampler(VKDevice *device, const SamplerCreateInfo &createInfo)
    : Sampler(createInfo), VKDeviceObject(device) {
    VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
                             nullptr,
                             0,
                             Map(createInfo.magFilter),
                             Map(createInfo.minFilter),
                             Map(createInfo.mipmapMode),
                             Map(createInfo.wrapModeU),
                             Map(createInfo.wrapModeV),
                             Map(createInfo.wrapModeW),
                             createInfo.mipLodBias,
                             createInfo.anisotropyEnable,
                             createInfo.maxAnisotropy,  // max anisotropy
                             createInfo.compareEnable,
                             Map(createInfo.compareOp),
                             createInfo.minLod,
                             createInfo.maxLod,
                             Map(createInfo.borderColor),
                             VK_FALSE};

    // VkSamplerCustomBorderColorCreateInfoEXT borderColor{
    //	VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
    //	nullptr,
    // };
    // memcpy(&borderColor.customBorderColor, &createInfo.borderColor, 32);

    // info.pNext = &borderColor;
    if (vkCreateSampler(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create sampler");
}
}  // namespace Shit