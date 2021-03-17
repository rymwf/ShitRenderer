/**
 * @file GLCommandPool.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLCommandPool.h"
namespace Shit
{

	void GLCommandPool::CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, std::vector<CommandBuffer *> &commandBuffers)
	{
		commandBuffers.resize(createInfo.count);
		for (uint32_t i = 0; i < createInfo.count; ++i)
		{
			mCommandBuffers.emplace_back(std::make_unique<GLCommandBuffer>(mpStateManager, createInfo));
			commandBuffers[i] = (mCommandBuffers.back().get());
		}
	}
	void GLCommandPool::DestroyCommandBuffer(CommandBuffer *pCommandBuffer)
	{
		RemoveFromUniqueVector(mCommandBuffers, pCommandBuffer);
	}
}