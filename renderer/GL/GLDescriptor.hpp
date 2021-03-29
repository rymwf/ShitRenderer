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
#include <renderer/ShitDescriptor.hpp>
#include "GLPrerequisites.hpp"
#include "GLBufferView.hpp"

#define MAX_BINDING_NUM 32

namespace Shit
{
	class GLDescriptorSetLayout final : public DescriptorSetLayout
	{
	public:
		GLDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo)
			: DescriptorSetLayout(createInfo) {}
	};

	/**
	 * @brief TODO: dynamic uniform and storage buffer
	 * 
	 */
	class GLDescriptorSet final : public DescriptorSet
	{
	public:
		struct BindingAttribute
		{
			DescriptorType type;
			std::variant<
				std::monostate,
				ImageView *,
				DescriptorBufferInfo,
				BufferView *>
				val;
		};

		GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout);

		template <IsAnyOf<ImageView *, DescriptorBufferInfo, BufferView *> T>
		void Set(DescriptorType type, uint32_t binding, const std::vector<T> &values)
		{
			for (auto &&e : values)
			{
				mBindingAttributes[binding++] = BindingAttribute{
					type,
					e};
			}
		}
		constexpr decltype(auto) GetBindingAttributePtr() const
		{
			return &mBindingAttributes;
		}

	private:
		GLStateManager *mpStateManger;
		std::array<BindingAttribute, MAX_BINDING_NUM> mBindingAttributes;
	};
	class GLDescriptorPool final : public DescriptorPool
	{
		GLStateManager *mpStateManager;

	public:
		GLDescriptorPool(GLStateManager *pStateManager, const DescriptorPoolCreateInfo &createInfo)
			: DescriptorPool(createInfo), mpStateManager(pStateManager) {}

		void Allocate(const DescriptorSetAllocateInfo &createInfo, std::vector<DescriptorSet *> &descriptorSets) override;
	};
}
