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
#include "VKDevice.h"

namespace Shit
{

	class VKShader final : public Shader
	{
		VkShaderModule mHandle;

	public:
		VKShader(const ShaderCreateInfo &createInfo) : Shader(createInfo)
		{
			VkShaderModuleCreateInfo tempCreateInfo{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				createInfo.code.size(),
				reinterpret_cast<const uint32_t *>(createInfo.code.data())};

			if (vkCreateShaderModule(static_cast<VKDevice *>(createInfo.pDevice)->GetHandle(), &tempCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create shadermodule");
		}
		~VKShader() override
		{
			vkDestroyShaderModule(static_cast<VKDevice *>(mpDevice)->GetHandle(), mHandle, nullptr);
		}
		VkShaderModule GetHandle() const
		{
			return mHandle;
		}
	};
}
