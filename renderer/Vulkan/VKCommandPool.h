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
namespace Shit
{
	class VKCommandPool final : public CommandPool
	{
		VkCommandPool mHandle;
		VkDevice mDevice;

	public:
		VKCommandPool(VkDevice device, const CommandPoolCreateInfo &createInfo) : CommandPool(createInfo), mDevice(device)
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				nullptr,
				0,
				createInfo.queueFamilyIndex};
			if (vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create command pool");
		}
		constexpr VkCommandPool GetHandle() const
		{
			return mHandle;
		}
		~VKCommandPool() override
		{
			vkDestroyCommandPool(mDevice, mHandle, nullptr);
		}
	};
} // namespace Shit
