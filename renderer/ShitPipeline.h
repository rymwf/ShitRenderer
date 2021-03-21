/**
 * @file ShitPipeline.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"

namespace Shit
{

	class PipelineLayout
	{
	protected:
		PipelineLayoutCreateInfo mCreateInfo;
		PipelineLayout(const PipelineLayoutCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~PipelineLayout() {}
		constexpr const PipelineLayoutCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};

	class Pipeline
	{
	public:
		virtual ~Pipeline() {}
	};

	class GraphicsPipeline : public virtual Pipeline
	{
	protected:
		GraphicsPipelineCreateInfo mCreateInfo;
		GraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~GraphicsPipeline() {}
		const GraphicsPipelineCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};

	class ComputePipeline : public virtual Pipeline
	{
	protected:
		ComputePipelineCreateInfo mCreateInfo;
		ComputePipeline(const ComputePipelineCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~ComputePipeline() {}
		const ComputePipelineCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
}
