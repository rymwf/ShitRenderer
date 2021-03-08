/**
 * @file VKPipeline.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKPipeline.h"
#include "VKDescriptor.h"
namespace Shit
{

	VKPipelineLayout::VKPipelineLayout(VkDevice device, const PipelineLayoutCreateInfo &createInfo)
		: PipelineLayout(createInfo), mDevice(device)
	{
		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<VkPushConstantRange> pushConstantRanges;
		for(auto&& e:createInfo.setLayouts)
			setLayouts.emplace_back(static_cast<VKDescriptorSetLayout *>(e)->GetHandle());
		for (auto &&e : createInfo.pushConstantRanges)
		{
			pushConstantRanges.emplace_back(
				VkPushConstantRange{
					Map(e.stageFlags),
					e.offset,
					e.size});
		}

		VkPipelineLayoutCreateInfo info{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(setLayouts.size()),
			setLayouts.data(),
			static_cast<uint32_t>(pushConstantRanges.size()),
			pushConstantRanges.data()};

		if (vkCreatePipelineLayout(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create pipelinelayout");
	}

	VKGraphicsPipeline::VKGraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo &createInfo)
		: GraphicsPipeline(createInfo), mDevice(device)
	{
	}
} // namespace Shit
