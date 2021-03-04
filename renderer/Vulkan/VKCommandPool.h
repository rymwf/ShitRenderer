/**
 * @file VKCommandPool.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandPool.h>
#include "VKPrerequisites.h"
#include "VKDevice.h"
namespace Shit
{
	class VKCommandPool final : public CommandPool
	{
		VkCommandPool mHandle;

	public:
		VKCommandPool(const CommandPoolCreateInfo &createInfo) : CommandPool(createInfo)
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				nullptr,
				0,
				createInfo.queueFamilyIndex};
			if (vkCreateCommandPool(static_cast<VKDevice *>(createInfo.pDevice)->GetHandle(), &commandPoolCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create command pool");
		}
		VkCommandPool GetHandle() const
		{
			return mHandle;
		}
		~VKCommandPool() override
		{
			vkDestroyCommandPool(static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);
		}
	};
} // namespace Shit
