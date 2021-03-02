/**
 * @file ShitShader.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"

namespace Shit
{
	class Shader
	{
		ShaderStageFlagBits mStageFlagBit;

	public:
		Shader(const ShaderModuleCreateInfo &createInfo) : mStageFlagBit(createInfo.stage) {}
		virtual ~Shader() {}
	};
}
