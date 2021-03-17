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
		VkPipelineLayout mHandle{};
		VkDevice mDevice;

	public:
		VKPipelineLayout(VkDevice device, const PipelineLayoutCreateInfo &createInfo);
		~VKPipelineLayout() override
		{
			vkDestroyPipelineLayout(mDevice, mHandle, nullptr);
		}
		constexpr VkPipelineLayout GetHandle() const
		{
			return mHandle;
		}
	};

	class VKPipeline : public virtual Pipeline
	{
	protected:
		VkPipeline mHandle;
		VkDevice mDevice;

	public:
		VKPipeline(VkDevice device) : mDevice(device) {}
		~VKPipeline() override
		{
			vkDestroyPipeline(mDevice, mHandle, nullptr);
		}
		constexpr VkPipeline GetHandle() const
		{
			return mHandle;
		}
	};

	class VKGraphicsPipeline final : public GraphicsPipeline, public VKPipeline
	{
	public:
		VKGraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo &createInfo);
		~VKGraphicsPipeline() override
		{
		}
		constexpr VkPipeline GetHandle() const
		{
			return mHandle;
		}
	};
}
