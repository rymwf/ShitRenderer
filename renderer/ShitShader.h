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
		ShaderCreateInfo mCreateInfo;

	public:
		Shader(const ShaderCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Shader() {}
		constexpr const ShaderCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
}
