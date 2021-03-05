/**
 * @file VKSemaphore.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSemaphore.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKSemaphore final : public Semaphore
	{
		VkSemaphore mHandle;
		VkDevice mDevice;

	public:
		VKSemaphore(VkDevice device, const SemaphoreCreateInfo &createInfo) : Semaphore(createInfo), mDevice(device) {}
		~VKSemaphore() override
		{
			vkDestroySemaphore(mDevice, mHandle, nullptr);
		}
		constexpr VkSemaphore GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
