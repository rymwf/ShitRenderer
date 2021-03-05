/**
 * @file VKBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKBuffer.h"
namespace Shit
{
	VKBuffer::VKBuffer(VkDevice device, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo, void *pData)
		: Buffer(createInfo), mDevice(device), mPhysicalDevice(physicalDevice)
	{
		VK::createBuffer(
			mPhysicalDevice,
			device,
			createInfo.size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | Map(createInfo.usage),
			Map(createInfo.memoryPropertyFlags),
			mHandle,
			mMemory);
	}
	void VKBuffer::MapBuffer(uint64_t offset, uint64_t size, void **ppData)
	{
		vkMapMemory(mDevice, mMemory, offset, size, 0, ppData);
	}
	void VKBuffer::UnMapBuffer()
	{
		vkUnmapMemory(mDevice, mMemory);
	}
	void VKBuffer::SetData(void *pData)
	{
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VK::createBuffer(
			mPhysicalDevice,
			mDevice,
			mCreateInfo.size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void *data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, mCreateInfo.size, 0, &data);
		memcpy(data, pData, static_cast<size_t>(mCreateInfo.size));
		vkUnmapMemory(mDevice, stagingBufferMemory);
		//

	}
} // namespace Shit
