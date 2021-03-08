/**
 * @file ShitDescriptor.h
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

	class DescriptorSetLayout
	{
	protected:
		DescriptorSetLayoutCreateInfo mCreateInfo;
		DescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~DescriptorSetLayout() {}
	};

	class DescriptorSet
	{
	};

}