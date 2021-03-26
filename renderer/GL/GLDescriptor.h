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
#include "GLBufferView.h"
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
		using BindingAttributes = std::variant<
			std::monostate,
			std::vector<ImageView *>,
			std::vector<DescriptorBufferInfo>,
			std::vector<BufferView *>>;

		GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout);

		template <IsAnyOf<ImageView *, DescriptorBufferInfo, BufferView *> T>
		void Set(DescriptorType type, uint32_t binding, const std::vector<T> &values)
		{
			auto &&a = mBindingAttributes[static_cast<size_t>(type)];
			if (a.index() == 0)
			{
				std::vector<T> b(binding + values.size());
				std::copy(values.begin(), values.end(), b.begin() + binding);
				a = std::move(b);
			}
			else
			{
				auto &&b = std::get<std::vector<T>>(a);
				b.resize((std::max)(b.size(), binding + values.size()));
				std::copy(values.begin(), values.end(), b.begin() + binding);
			}
		}

		//template <IsAnyOf<ImageView *, DescriptorBufferInfo, BufferView *> T>
		//void Set(DescriptorType type, uint32_t binding, std::vector<T>::iterator first, std::vector<T>::iterator last)
		////void Set(DescriptorType type, uint32_t binding, std::vector<T>::iterator first, std::vector<T>::iterator last)
		//{
		//	auto &&a = mBindingAttributes[static_cast<size_t>(type)];
		//	if (a.index() == 0)
		//	{
		//		std::vector<T> b(last - first + binding, nullptr);
		//		std::copy(b.begin() + binding, first, last);
		//		a = std::move(b);
		//	}
		//	else
		//	{
		//		auto &&b = std::get<std::vector<T>>(a);
		//		b.resize((std::max)(b.size(), last - first + binding));
		//		std::copy(b.begin() + binding, first, last);
		//	}
		//}

		constexpr decltype(auto) GetBindingAttributePtr() const
		{
			return &mBindingAttributes;
		}

	private:
		GLStateManager *mpStateManger;
		//std::array<std::vector<BindingAttribute>, static_cast<size_t>(DescriptorType::Num)> mBindingAttributes;
		std::array<BindingAttributes, static_cast<size_t>(DescriptorType::Num)> mBindingAttributes;
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
