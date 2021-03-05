/**
 * @file VKFence.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitFence.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKFence final : public Fence
	{
		VkFence mHandle;
		VkDevice mDevice;

	public:
		VKFence(VkDevice device, const FenceCreateInfo &createInfo) : Fence(createInfo), mDevice(device) {}
		~VKFence() override
		{
			vkDestroyFence(mDevice, mHandle, nullptr);
		}
		constexpr VkFence GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
