/**
 * @file VKSampler.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKSampler.h"
namespace Shit
{
	VKSampler::VKSampler(VkDevice device, const SamplerCreateInfo &createInfo)
		: Sampler(createInfo), mDevice(device)
	{
		VkSamplerCreateInfo info{
			VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			nullptr,
			0,
			Map(createInfo.magFilter),
			Map(createInfo.minFilter),
			Map(createInfo.mipmapMode),
			Map(createInfo.wrapModeU),
			Map(createInfo.wrapModeV),
			Map(createInfo.wrapModeW),
			createInfo.mipLodBias,
			createInfo.anisotopyEnable,
			16, //max anisotropy
			createInfo.compareEnable,
			Map(createInfo.compareOp),
			createInfo.minLod,
			createInfo.maxLod,
			//VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
			VK_BORDER_COLOR_FLOAT_CUSTOM_EXT,
			VK_FALSE};

		VkSamplerCustomBorderColorCreateInfoEXT borderColor{
			VK_STRUCTURE_TYPE_SAMPLER_CUSTOM_BORDER_COLOR_CREATE_INFO_EXT,
			nullptr,
		};
		if (createInfo.borderColor.dataType == DataType::INT)
		{
			borderColor.format = VK_FORMAT_R32G32B32A32_SINT;
			memcpy(borderColor.customBorderColor.int32, createInfo.borderColor.color.int32, 32);
		}
		else if (createInfo.borderColor.dataType == DataType::UNSIGNED_INT)
		{
			borderColor.format = VK_FORMAT_R32G32B32A32_UINT;
			memcpy(borderColor.customBorderColor.uint32, createInfo.borderColor.color.uint32, 32);
		}
		else if (createInfo.borderColor.dataType == DataType::FLOAT)
		{
			borderColor.format = VK_FORMAT_R32G32B32A32_SFLOAT;
			memcpy(borderColor.customBorderColor.float32, createInfo.borderColor.color.float32, 32);
		}
		info.pNext = &borderColor;
		if (vkCreateSampler(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create sampler");
	}
}