/**
 * @file VKSampler.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSampler.hpp>
#include "VKPrerequisites.hpp"
#include "VKDevice.hpp"
namespace Shit
{
	class VKSampler final : public Sampler
	{
		VkSampler mHandle;
		VkDevice mDevice;

	public:
		VKSampler(VkDevice device, const SamplerCreateInfo &createInfo);
		~VKSampler() override
		{
			vkDestroySampler(mDevice, mHandle, nullptr);
		}
		constexpr VkSampler GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
