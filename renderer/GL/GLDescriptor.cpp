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
	void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, const std::vector<ImageView *> &values)
	{
		if (type == DescriptorType::COMBINED_IMAGE_SAMPLER)
		{
			for (auto &&e : values)
			{
				mBindingTextures[binding++] = {type, (void *)e};
			}
		}
		else if (type == DescriptorType::STORAGE_IMAGE)
		{
			for (auto &&e : values)
			{
				mBindingImages[binding++] = {type, (void *)e};
			}
		}
		else
		{
			LOG("descriptor type not supported yet");
		}
	}
	void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, const std::vector<BufferView *> &values)
	{
		if (type == DescriptorType::UNIFORM_TEXEL_BUFFER)
		{
			for (auto &&e : values)
			{
				mBindingTextures[binding++] = {type, (void *)e};
			}
		}
		else if (type == DescriptorType::STORAGE_TEXEL_BUFFER)
		{
			for (auto &&e : values)
			{
				mBindingImages[binding++] = {type, (void *)e};
			}
		}
	}
	void GLDescriptorSet::Set(DescriptorType type, uint32_t binding, const std::vector<DescriptorBufferInfo> &values)
	{
		if (type == DescriptorType::UNIFORM_BUFFER || type == DescriptorType::UNIFORM_BUFFER_DYNAMIC)
		{
			for (auto &&e : values)
			{
				mBindingUniformBuffers[binding++] = {type, e};
			}
		}
		else if (type == DescriptorType::STORAGE_BUFFER || type == DescriptorType::STORAGE_BUFFER_DYNAMIC)
		{
			for (auto &&e : values)
			{
				mBindingStorageBuffers[binding++] = {type, e};
			}
		}
	}
	//===========================================================================================
	void GLDescriptorPool::Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets)
	{
		auto count = createInfo.setLayouts.size();
		descriptorSets.resize(count);
		for (size_t i = 0; i < count; ++i)
		{
			mDescriptorSets.emplace_back(std::make_unique<GLDescriptorSet>(mpStateManager, createInfo.setLayouts[i]));
			descriptorSets[i] = mDescriptorSets.back().get();
		}
	}
} // namespace Shit
