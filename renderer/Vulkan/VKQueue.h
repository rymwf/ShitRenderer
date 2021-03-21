/**
 * @file VKQueue.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitQueue.h>
#include "VKPrerequisites.h"
namespace Shit
{
	class VKQueue final : public Queue
	{
		VkQueue mHandle;
		VkDevice mDevice;

	public:
		VKQueue(VkDevice device, const QueueCreateInfo &createInfo) : Queue(createInfo), mDevice(device)
		{
			vkGetDeviceQueue(mDevice, createInfo.queueFamilyIndex, createInfo.queueIndex, &mHandle);
		}
		constexpr VkQueue GetHandle() const
		{
			return mHandle;
		}
		void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) override;

		Result Present(const PresentInfo &presentInfo) override;

		void WaitIdle() override;
	};
} // namespace Shit
