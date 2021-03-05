/**
 * @file VKCommandBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandBuffer.h>
#include "VKPrerequisites.h"
#include "VKCommandPool.h"
namespace Shit
{

	class VKCommandBuffer final : public CommandBuffer
	{
		VkCommandBuffer mHandle;
		VkDevice mDevice;

	public:
		VKCommandBuffer(VkDevice device, const CommandBufferCreateInfo &createInfo) : CommandBuffer(createInfo), mDevice(device)
		{
			VkCommandBufferAllocateInfo allocateInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				nullptr,
				static_cast<VKCommandPool *>(createInfo.pCommandPool)->GetHandle(),
				Map(createInfo.level),
				1};
			if (vkAllocateCommandBuffers(mDevice, &allocateInfo, &mHandle) != VK_SUCCESS)
				THROW("failed to create command buffer");
		}
		constexpr VkCommandBuffer GetHandle()
		{
			return mHandle;
		}
		~VKCommandBuffer() override
		{
		}
	};
}
