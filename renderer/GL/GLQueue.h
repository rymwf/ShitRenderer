/**
 * @file GLQueue.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitQueue.h>
#include "GLPrerequisites.h"

namespace Shit
{
	class GLQueue final : public Queue
	{
		GLStateManager *mpStateManager;
		Device *mpDevice;

	public:
		GLQueue(Device *pDevice, GLStateManager *pStateManager, const QueueCreateInfo &createInfo)
			: Queue(createInfo), mpStateManager(pStateManager), mpDevice(pDevice) {}

		void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *pFence) override;
		void Present(const PresentInfo &presentInfo) override;
		void WaitIdle() override
		{
			glFinish();
		}
	};
} // namespace Shit
