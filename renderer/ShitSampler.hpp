/**
 * @file ShitSampler.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit
{
	class Sampler
	{
	protected:
		SamplerCreateInfo mCreateInfo;

	public:
		Sampler(const SamplerCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Sampler() {}
		constexpr const SamplerCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
