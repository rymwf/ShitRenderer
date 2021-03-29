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
#include <renderer/ShitDescriptor.hpp>
#include "VKPrerequisites.hpp"
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
		VkDescriptorSet mHandle;
		VkDescriptorPool mPool;
		VkDevice mDevice;

	public:
		VKDescriptorSet(VkDevice device, VkDescriptorPool pool, const DescriptorSetLayout *pSetlayout);
		~VKDescriptorSet() override
		{
			//vkFreeDescriptorSets(mDevice, mPool, 1, &mHandle);
		}
		constexpr VkDescriptorSet GetHandle() const
		{
			return mHandle;
		}
	};
	class VKDescriptorPool final : public DescriptorPool
	{
		VkDescriptorPool mHandle;
		VkDevice mDevice;

	public:
		VKDescriptorPool(VkDevice device, const DescriptorPoolCreateInfo &createInfo);
		~VKDescriptorPool() override
		{
			mDescriptorSets.clear();
			vkDestroyDescriptorPool(mDevice, mHandle, nullptr);
		}
		void Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets) override;
	};
} // namespace Shit
