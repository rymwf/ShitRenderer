/**
 * @file VKDescriptor.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDescriptor.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKDescriptorSetLayout final : public DescriptorSetLayout
	{
		VkDescriptorSetLayout mHandle;
		VkDevice mDevice;

	public:
		VKDescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo &createInfo);
		~VKDescriptorSetLayout() override
		{
			vkDestroyDescriptorSetLayout(mDevice, mHandle, nullptr);
		}

		constexpr VkDescriptorSetLayout GetHandle() const
		{
			return mHandle;
		}
	};
	class VKDescriptorSet final : public DescriptorSet
	{
	};
} // namespace Shit
