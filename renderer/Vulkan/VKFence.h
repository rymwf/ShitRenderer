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
#include "VKDevice.h"
namespace Shit
{
	class VKFence final : public Fence
	{
		VkFence mHandle;

	public:
		VKFence(const FenceCreateInfo &createInfo) : Fence(createInfo) {}
		~VKFence() override
		{
			vkDestroyFence(static_cast<VKDevice *>(mCreateInfo.pDevice)->GetHandle(), mHandle, nullptr);
		}
	};
} // namespace Shit
