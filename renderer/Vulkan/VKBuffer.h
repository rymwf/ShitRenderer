/**
 * @file VKBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBuffer.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKBuffer final : public Buffer
	{
		VkBuffer mHandle;
		VkDeviceMemory mMemory;
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		void SetData(void* pData);

	public:
		VKBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo, void *pData);
		~VKBuffer() override
		{
			vkFreeMemory(mDevice, mMemory, nullptr);
			vkDestroyBuffer(mDevice, mHandle, nullptr);
		}
		constexpr VkBuffer GetHandle() const
		{
			return mHandle;
		}
		void MapBuffer(uint64_t offset, uint64_t size, void **ppData) override;
		void UnMapBuffer() override;
	};

}
