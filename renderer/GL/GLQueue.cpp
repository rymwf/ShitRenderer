/**
 * @file GLQueue.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLQueue.h"
#include "GLCommandBuffer.h"
#include "GLFence.h"
#include "GLSwapchain.h"
#include "GLSemaphore.h"
#include "GLFramebuffer.h"

namespace Shit
{
	void GLQueue::Submit(const std::vector<SubmitInfo> &submitInfos, Fence *pFence)
	{
		for (auto &&submitInfo : submitInfos)
		{
			for (auto &&waitSemaphore : submitInfo.waitSempahores)
			{
				static_cast<GLSemaphore *>(waitSemaphore)->Wait();
			}
			for (auto &&commandBuffer : submitInfo.commandBuffers)
			{
				static_cast<GLCommandBuffer *>(commandBuffer)->Execute();
			}
			for (auto &&signalSemaphore : submitInfo.signalSempahores)
			{
				static_cast<GLSemaphore *>(signalSemaphore)->Reset();
			}
		}
		if (pFence)
			static_cast<GLFence *>(pFence)->Reset();
	}

	Result GLQueue::Present(const PresentInfo &presentInfo)
	{
		for (auto &&e : presentInfo.waitSemaphores)
		{
			static_cast<GLSemaphore *>(e)->Wait();
		}
		for (size_t i = 0, len = presentInfo.swapchains.size(); i < len; ++i)
		{
			auto pSwapchain = static_cast<GLSwapchain *>(presentInfo.swapchains[i]);
			auto &&swapchainExtent = pSwapchain->GetCreateInfoPtr()->imageExtent;
			auto pFrambuffer = const_cast<GLFramebuffer *>(static_cast<const GLFramebuffer *>(pSwapchain->GetFramebufferPtr()));

			mpStateManager->BindReadFramebuffer(pFrambuffer->GetHandle());
			pFrambuffer->BindReadBuffer(presentInfo.imageIndices[i]);
			mpStateManager->BindDrawFramebuffer(0);
			glBlitFramebuffer(0, 0, swapchainExtent.width, swapchainExtent.height,
							  0, 0, swapchainExtent.width, swapchainExtent.height,
							  GL_COLOR_BUFFER_BIT,
							  GL_NEAREST);
			pSwapchain->SwapBuffer();
		}
		return Result::SUCCESS;
	}
}
