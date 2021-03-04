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
#include "ShitRendererPrerequisites.h"
#include "ShitCommandBuffer.h"
namespace Shit
{
	class Queue
	{
		public:
			virtual ~Queue() {}
			virtual void Submit(const std::vector<SubmitInfo> &submitInfos, Fence *fence) = 0;
	};
} // namespace Shit
