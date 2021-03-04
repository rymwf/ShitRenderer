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

namespace Shit{

	class GraphicsPipeline
	{
	protected:
		GraphicsPipelineCreateInfo mCreateInfo;

	public:
		GraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) : mCreateInfo(createInfo)
		{
		}
		virtual ~GraphicsPipeline(){}
	};

	class ComputePipeline
	{
	};
}

