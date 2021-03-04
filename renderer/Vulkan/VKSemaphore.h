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
#include "VKDevice.h"
namespace Shit
{
	class VKSemaphore final : public Semaphore
	{
		VkSemaphore mHandle;

	public:
		VKSemaphore(const SemaphoreCreateInfo &createInfo) : Semaphore(createInfo) {}
		~VKSemaphore() override
		{
			vkDestroySemaphore(static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);
		}
	};
} // namespace Shit
