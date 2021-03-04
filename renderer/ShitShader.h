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
	protected:
		ShaderStageFlagBits mStageFlagBit;
		Device *mpDevice;

	public:
		Shader(const ShaderCreateInfo &createInfo) : mStageFlagBit(createInfo.stage), mpDevice(createInfo.pDevice) {}
		virtual ~Shader() {}
	};
}
