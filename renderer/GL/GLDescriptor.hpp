/**
 * @file GLDescriptor.hpp
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
		GLDescriptorSet(GLStateManager *pStateManager, const DescriptorSetLayout *pDescriptorSetLayout);

		void Set(DescriptorType type, uint32_t binding, const std::vector<ImageView *> &values);
		void Set(DescriptorType type, uint32_t binding, const std::vector<BufferView *> &values);
		void Set(DescriptorType type, uint32_t binding, const std::vector<DescriptorBufferInfo> &values);

		constexpr decltype(auto) GetBindingTextures() const
		{
			return &mBindingTextures;
		}
		constexpr decltype(auto) GetBindingImages() const
		{
			return &mBindingImages;
		}
		constexpr decltype(auto) GetBindingUniformBuffers() const
		{
			return &mBindingUniformBuffers;
		}
		constexpr decltype(auto) GetBindingStorageBuffers() const
		{
			return &mBindingStorageBuffers;
		}

	private:
		GLStateManager *mpStateManger;

		std::unordered_map<uint32_t, std::pair<DescriptorType, void *>> mBindingTextures;					  //combined textures and buffer views
		std::unordered_map<uint32_t, std::pair<DescriptorType, void *>> mBindingImages;						  //images and bufferviews
		std::unordered_map<uint32_t, std::pair<DescriptorType, DescriptorBufferInfo>> mBindingUniformBuffers; //uniform buffer
		std::unordered_map<uint32_t, std::pair<DescriptorType, DescriptorBufferInfo>> mBindingStorageBuffers;
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
