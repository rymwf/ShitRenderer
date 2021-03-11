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

		if (vkCreateDescriptorSetLayout(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create descriptor setlayout");
	}

}