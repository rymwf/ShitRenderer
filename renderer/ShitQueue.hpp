/**
 * @file ShitQueue.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitCommandBuffer.hpp"
namespace Shit
{
	class Queue
	{
	protected:
		QueueCreateInfo mCreateInfo;
		Queue(const QueueCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~Queue() {}
		virtual void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) = 0;
		virtual Result Present(const PresentInfo &presentInfo) = 0;
		virtual void WaitIdle() = 0;
		constexpr const QueueCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
