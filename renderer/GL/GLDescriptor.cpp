/**
 * @file GLDescriptor.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLDescriptor.hpp"
namespace Shit
{
	GLDescriptorSet::GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout)
		: DescriptorSet(pDescriptorSetLayout), mpStateManger(pStateManager)
	{
	}
	//===========================================================================================
	void GLDescriptorPool::Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets)
	{
		LOG("NOTE: in opengl only ONE descriptor is allowed");
		auto count = (std::min)(createInfo.setLayouts.size(), size_t(1));
		descriptorSets.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			mDescriptorSets.emplace_back(std::make_unique<GLDescriptorSet>(mpStateManager, createInfo.setLayouts[i]));
			descriptorSets[i] = mDescriptorSets.back().get();
		}
	}
} // namespace Shit
