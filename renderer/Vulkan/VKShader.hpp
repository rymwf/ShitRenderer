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
#include "VKPrerequisites.hpp"

namespace Shit
{

	class VKShader final : public Shader
	{
		VkShaderModule mHandle;
		VkDevice mDevice;

	public:
		VKShader(VkDevice device, const ShaderCreateInfo &createInfo) : Shader(createInfo), mDevice(device)
		{
			VkShaderModuleCreateInfo tempCreateInfo{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				createInfo.code.size(),
				reinterpret_cast<const uint32_t *>(createInfo.code.data())};

			if (vkCreateShaderModule(mDevice, &tempCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create shadermodule");
		}
		~VKShader() override
		{
			vkDestroyShaderModule(mDevice, mHandle, nullptr);
		}
		constexpr VkShaderModule GetHandle() const
		{
			return mHandle;
		}
	};
}
