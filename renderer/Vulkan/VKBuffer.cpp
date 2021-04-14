/**
 * @file VKBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKBuffer.hpp"
#include "VKQueue.hpp"
#include "VKCommandPool.hpp"
#include "VKDevice.hpp"

namespace Shit
{
	VKBuffer::VKBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo)
		: Buffer(createInfo), mDevice(device), mPhysicalDevice(physicalDevice)
	{
		VkBufferCreateInfo info{
			VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			nullptr,
			0,
			createInfo.size,
			Map(createInfo.usage),
			VK_SHARING_MODE_EXCLUSIVE,
		};
		if (vkCreateBuffer(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create vertex buffer");

		VkMemoryRequirements vertexBufferMemoryRequirements;
		vkGetBufferMemoryRequirements(mDevice, mHandle, &vertexBufferMemoryRequirements);

//		LOG_VAR(vertexBufferMemoryRequirements.size);
//		LOG_VAR(vertexBufferMemoryRequirements.alignment);
//		LOG_VAR(vertexBufferMemoryRequirements.memoryTypeBits); //typebits is the memorytype indices in physical memory properties

		auto memoryTypeIndex = VK::findMemoryTypeIndex(physicalDevice, vertexBufferMemoryRequirements.memoryTypeBits, Map(createInfo.memoryPropertyFlags)); //the index of memory type

		mMemory= VK::allocateMemory(mDevice, vertexBufferMemoryRequirements.size, memoryTypeIndex);

		vkBindBufferMemory(mDevice, mHandle, mMemory, 0);
	}
	void VKBuffer::MapMemory(uint64_t offset, uint64_t size, void **ppData)
	{
		vkMapMemory(mDevice, mMemory, offset, size, 0, ppData);
	}
	void VKBuffer::UnMapMemory()
	{
		vkUnmapMemory(mDevice, mMemory);
	}
} // namespace Shit
