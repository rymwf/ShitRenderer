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
#include "VKDevice.h"
namespace Shit
{

	class VKCommandBuffer final : public CommandBuffer
	{
		VkCommandBuffer mHandle;

	public:
		VKCommandBuffer(const CommandBufferCreateInfo &createInfo) : CommandBuffer(createInfo)
		{
			VkCommandBufferAllocateInfo allocateInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				nullptr,
				static_cast<VKCommandPool *>(createInfo.pCommandPool)->GetHandle(),
				Map(createInfo.level),
				1};
			if (vkAllocateCommandBuffers(static_cast<VKDevice *>(createInfo.pDevice)->GetHandle(), &allocateInfo, &mHandle) != VK_SUCCESS)
				THROW("failed to create command buffer");
		}
		VkCommandBuffer GetHandle()
		{
			return mHandle;
		}
		~VKCommandBuffer() override
		{
		}
	};
}
