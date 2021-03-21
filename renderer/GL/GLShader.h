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
		}
	};
}