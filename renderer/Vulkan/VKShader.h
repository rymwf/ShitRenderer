/**
 * @file VKShader.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitShader.h>
#include "VKPrerequisites.h"

namespace Shit
{

	class VKShader final : public Shader
	{
		VkShaderModule mHandle;

	public:
		VKShader(VkDevice device, const ShaderModuleCreateInfo &createInfo) : Shader(createInfo)
		{
			VkShaderModuleCreateInfo tempCreateInfo{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				createInfo.code.size(),
				reinterpret_cast<const uint32_t *>(createInfo.code.data())};

			if (vkCreateShaderModule(device, &tempCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create shadermodule");
		}
		VkShaderModule GetHandle() const
		{
			return mHandle;
		}
	};
}
