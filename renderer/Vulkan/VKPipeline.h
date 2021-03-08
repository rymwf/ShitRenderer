/**
 * @file VKPipeline.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitPipeline.h>
#include "VKPrerequisites.h"
namespace Shit
{

	class VKPipelineLayout final : public PipelineLayout
	{
		VkPipelineLayout mHandle;
		VkDevice mDevice;

	public:
		VKPipelineLayout(VkDevice device, const PipelineLayoutCreateInfo &createInfo);
		~VKPipelineLayout()
		{
			vkDestroyPipelineLayout(mDevice, mHandle, nullptr);
		}
	};

	class VKGraphicsPipeline final : public GraphicsPipeline
	{
		VkPipeline mHandle;
		VkDevice mDevice;

	public:
		VKGraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo &createInfo);
		~VKGraphicsPipeline()
		{
			vkDestroyPipeline(mDevice, mHandle, nullptr);
		}
		constexpr VkPipeline GetHandle() const
		{
			return mHandle;
		}
	};
}
