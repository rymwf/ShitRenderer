/**
 * @file VKSemaphore.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSemaphore.hpp>
#include "VKPrerequisites.hpp"
namespace Shit
{
	class VKSemaphore final : public Semaphore
	{
		VkSemaphore mHandle;
		VkDevice mDevice;

	public:
		VKSemaphore(VkDevice device, const SemaphoreCreateInfo &createInfo)
			: Semaphore(createInfo), mDevice(device)
		{
			VkSemaphoreCreateInfo info{
				VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};
			if (vkCreateSemaphore(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create semaphore");
		}
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
