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
#include "VKDevice.h"
namespace Shit
{
	class VKQueue final : public Queue
	{
		VkQueue mHandle;

	public:
		VKQueue(const QueueCreateInfo &createInfo) : Queue(createInfo)
		{
			vkGetDeviceQueue(static_cast<VKDevice *>(createInfo.pDevice)->GetHandle(), createInfo.queueFamilyIndex, createInfo.queueIndex, &mHandle);
		}
		VkQueue GetHandle() const
		{
			return mHandle;
		}
		void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) override;
	};
} // namespace Shit
