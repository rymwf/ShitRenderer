/**
 * @file ShitShader.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitNonCopyable.hpp"

namespace Shit
{
	class Shader : public NonCopyable
	{
	protected:
		ShaderCreateInfo mCreateInfo;

	public:
		Shader(const ShaderCreateInfo &createInfo)
		{
			mCreateInfo.size = createInfo.size;
			mCreateInfo.code = new char[mCreateInfo.size];
			memcpy(mCreateInfo.code, createInfo.code, mCreateInfo.size);
		}
		virtual ~Shader()
		{
			delete[] mCreateInfo.code;
		}
		constexpr const ShaderCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
}
