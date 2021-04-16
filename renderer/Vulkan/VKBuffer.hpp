/**
 * @file VKBuffer.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBuffer.hpp>
#include "VKPrerequisites.hpp"
namespace Shit
{
	class VKBuffer final : public Buffer
	{
		VkBuffer mHandle;
		VkDeviceMemory mMemory;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

	public:
		VKBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo);
		~VKBuffer() override
		{
			vkFreeMemory(mDevice, mMemory, nullptr);
			vkDestroyBuffer(mDevice, mHandle, nullptr);
		}
		constexpr VkBuffer GetHandle() const
		{
			return mHandle;
		}
		void MapMemory(uint64_t offset, uint64_t size, void **ppData) override;
		void UnMapMemory() override;
		void FlushMappedMemoryRange(uint64_t offset, uint64_t size) override;
	};

}
