/**
 * @file GLSampler.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLSampler.h"
namespace Shit
{
	GLSampler::GLSampler(GLStateManager *pStateManager, const SamplerCreateInfo &createInfo)
		: Sampler(createInfo), mpStateManager(pStateManager)
	{
		glGenSamplers(1, &mHandle);
		glSamplerParameteri(mHandle, GL_TEXTURE_MAG_FILTER, Map(createInfo.magFilter));
		if (createInfo.mipmapMode == SamplerMipmapMode::LINEAR)
		{
			if (createInfo.minFilter == Filter::LINEAR)
				glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			else
				glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		}
		else
		{
			if (createInfo.minFilter == Filter::LINEAR)
				glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			else
				glSamplerParameteri(mHandle, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
		glSamplerParameterf(mHandle, GL_TEXTURE_LOD_BIAS, createInfo.mipLodBias);
		glSamplerParameterf(mHandle, GL_TEXTURE_MIN_LOD, createInfo.minLod);
		glSamplerParameterf(mHandle, GL_TEXTURE_MAX_LOD, createInfo.maxLod);
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_S, Map(createInfo.wrapModeU));
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_T, Map(createInfo.wrapModeV));
		glSamplerParameteri(mHandle, GL_TEXTURE_WRAP_R, Map(createInfo.wrapModeW));
		if (createInfo.compareEnable)
		{
			glSamplerParameteri(mHandle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glSamplerParameteri(mHandle, GL_TEXTURE_COMPARE_FUNC, Map(createInfo.compareOp));
		}
		if (createInfo.borderColor.index() == 0)
			glSamplerParameterfv(mHandle, GL_TEXTURE_BORDER_COLOR, std::get<0>(createInfo.borderColor).data());
		else if (createInfo.borderColor.index() == 1)
			glSamplerParameterIiv(mHandle, GL_TEXTURE_BORDER_COLOR, std::get<1>(createInfo.borderColor).data());
		else if (createInfo.borderColor.index() == 2)
			glSamplerParameterIuiv(mHandle, GL_TEXTURE_BORDER_COLOR, std::get<2>(createInfo.borderColor).data());
	}
}