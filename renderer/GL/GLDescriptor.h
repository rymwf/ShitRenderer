/**
 * @file GLDescriptor.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDescriptor.h>
#include "GLPrerequisites.h"
namespace Shit
{
	class GLDescriptorSetLayout final : public DescriptorSetLayout
	{
	public:
		GLDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo)
			: DescriptorSetLayout(createInfo) {}
	};
}
