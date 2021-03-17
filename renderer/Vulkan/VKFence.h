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
		VKFence(VkDevice device, const FenceCreateInfo &createInfo) : Fence(createInfo), mDevice(device)
		{
			VkFenceCreateInfo info{
				VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				nullptr,
				Map(createInfo.flags)};
			CHECK_VK_RESULT(vkCreateFence(mDevice, &info, nullptr, &mHandle));
		}
		~VKFence() override
		{
			vkDestroyFence(mDevice, mHandle, nullptr);
		}
		constexpr VkFence GetHandle() const
		{
			return mHandle;
		}
		void Reset() override
		{
			vkResetFences(mDevice, 1, &mHandle);
		}
		Result WaitFor(uint64_t timeout) override
		{
			switch (vkWaitForFences(mDevice, 1, &mHandle, VK_TRUE, timeout))
			{
			case VK_SUCCESS:
				return Result::SUCCESS;
			case VK_TIMEOUT:
				return Result::TIMEOUT;
			default:
				return Result::SHIT_ERROR;
			}
		}
	};
} // namespace Shit
