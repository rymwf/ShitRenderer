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

	public:
		VKQueue(VkQueue queue) : mHandle(queue) {}
		VkQueue GetHandle() const
		{
			return mHandle;
		}
		void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) override
		{
		}
	};
} // namespace Shit
