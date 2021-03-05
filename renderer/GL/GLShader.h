/**
 * @file GLShader.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitShader.h>
#include "GLPrerequisites.h"
namespace Shit
{

	class GLShader final : public Shader
	{
		GLuint mHandle;
		GLStateManager* mStateManager;

	public:
		GLShader(GLStateManager* stateManger, const ShaderCreateInfo &createInfo) : Shader(createInfo), mStateManager(stateManger)
		{
			mHandle = glCreateShader(Map(createInfo.stage));
			if (mHandle)
			{
				glShaderBinary(1, &mHandle, GL_SHADER_BINARY_FORMAT_SPIR_V, createInfo.code.data(), createInfo.code.size());
			}
			else
				THROW("failed to create shader");
		}

		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
	};
}