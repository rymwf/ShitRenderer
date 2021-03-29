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
#include "ShitRendererPrerequisites.hpp"

namespace Shit
{

	class DescriptorSetLayout
	{
	protected:
		DescriptorSetLayoutCreateInfo mCreateInfo;
		DescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~DescriptorSetLayout() {}
		constexpr const DescriptorSetLayoutCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};

	class DescriptorSet
	{
	protected:
		const DescriptorSetLayout *mpSetLayout;
		DescriptorSet(const DescriptorSetLayout *pSetLayout) : mpSetLayout(pSetLayout) {}

	public:
		virtual ~DescriptorSet() {}
		constexpr const DescriptorSetLayout *GetSetLayoutPtr() const
		{
			return mpSetLayout;
		}
	};

	class DescriptorPool
	{
	protected:
		DescriptorPoolCreateInfo mCreateInfo;
		std::list<std::unique_ptr<DescriptorSet>> mDescriptorSets;

	public:
		DescriptorPool(const DescriptorPoolCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~DescriptorPool() {}

		/**
		 * @brief in opengl, only the first descriptor set works
		 * 
		 * @param createInfo 
		 * @param descriptorSets 
		 */
		virtual void Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets) = 0;
		constexpr const DescriptorPoolCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
		//void Free(const std::vector<DescriptorSet *> &descriptorSets)
		//{
		//	for (auto &&e : descriptorSets)
		//	{
		//		RemoveSmartPtrFromContainer(mDescriptorSets, e);
		//	}
		//}
	};
}