/**
 * @file VKDescriptor.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKDescriptor.h"
#include "VKSampler.h"
namespace Shit
{
	VKDescriptorSetLayout::VKDescriptorSetLayout(VkDevice device, const DescriptorSetLayoutCreateInfo &createInfo)
		: DescriptorSetLayout(createInfo), mDevice(device)
	{
		std::vector<VkDescriptorSetLayoutBinding> bindings;
		for (auto &&e : createInfo.descriptorSetLayoutBindings)
		{
			std::vector<VkSampler> samplers;
			for (auto &&s : e.immutableSamplers)
				samplers.emplace_back(static_cast<VKSampler *>(s)->GetHandle());

			bindings.emplace_back(
				VkDescriptorSetLayoutBinding{
					e.binding,
					Map(e.descriptorType),
					e.descriptorCount,
					static_cast<VkShaderStageFlags>(Map(e.stageFlags)),
					samplers.data()});
		}
		VkDescriptorSetLayoutCreateInfo info{
			VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(bindings.size()),
			bindings.data()};

		CHECK_VK_RESULT(vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &mHandle));
	}
	VKDescriptorSet::VKDescriptorSet(VkDevice device, VkDescriptorPool pool, const DescriptorSetLayout *pSetLayout)
		: DescriptorSet(pSetLayout), mDevice(device), mPool(pool)
	{
		auto a = static_cast<const VKDescriptorSetLayout *>(pSetLayout)->GetHandle();
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = mPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &a};
		CHECK_VK_RESULT(vkAllocateDescriptorSets(mDevice, &allocInfo, &mHandle));
	}

	VKDescriptorPool::VKDescriptorPool(VkDevice device, const DescriptorPoolCreateInfo &createInfo)
		: DescriptorPool(createInfo), mDevice(device)
	{
		std::vector<VkDescriptorPoolSize> poolSizes(createInfo.poolSizes.size());
		std::transform(std::execution::par, createInfo.poolSizes.begin(), createInfo.poolSizes.end(), poolSizes.begin(), [](auto &&e) {
			return VkDescriptorPoolSize{
				Map(e.type),
				e.count};
		});

		VkDescriptorPoolCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = createInfo.maxSets,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes = poolSizes.data()};

		CHECK_VK_RESULT(vkCreateDescriptorPool(mDevice, &info, nullptr, &mHandle));
	}
	void VKDescriptorPool::Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets)
	{
		auto count = createInfo.setLayouts.size();
		descriptorSets.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			mDescriptorSets.emplace_back(std::make_unique<VKDescriptorSet>(mDevice, mHandle, createInfo.setLayouts[i]));
			descriptorSets[i] = mDescriptorSets.back().get();
		}
	}
}