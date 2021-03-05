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

		GLStateManager *mStateManager;

	public:
		GLQueue(GLStateManager *stateManager, const QueueCreateInfo &createInfo)
			: Queue(createInfo), mStateManager(stateManager) {}
		void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) override
		{
		}
	};
} // namespace Shit
