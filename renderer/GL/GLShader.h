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

	public:
		GLShader(const ShaderCreateInfo &createInfo) : Shader(createInfo)
		{
			mHandle = glCreateShader(Map(createInfo.stage));
			if (mHandle)
			{
				glShaderBinary(1, &mHandle, GL_SHADER_BINARY_FORMAT_SPIR_V, createInfo.code.data(), createInfo.code.size());
			}
			else
				THROW("failed to create shader");
		}

		GLuint GetHandle() const
		{
			return mHandle;
		}
	};
}