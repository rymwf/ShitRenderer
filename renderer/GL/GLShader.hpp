/**
 * @file GLShader.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitShader.hpp>
#include "GLPrerequisites.hpp"
namespace Shit
{

	class GLShader final : public Shader
	{
	public:
		GLShader(const ShaderCreateInfo &createInfo) : Shader(createInfo)
		{
		}
	};
}