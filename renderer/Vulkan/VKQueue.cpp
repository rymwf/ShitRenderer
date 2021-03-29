/**
 * @file VKQueue.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKQueue.hpp"
#include "VKSemaphore.hpp"
#include "VKCommandBuffer.hpp"
#include "VKFence.hpp"
#include "VKSwapchain.hpp"

namespace Shit
{
	void VKQueue::Submit(const std::vector<SubmitInfo> &submitInfos, Fence *pFence)
	{
		std::vector<VkSemaphore> waitSemaphores{};
		std::vector<VkPipelineStageFlags> waitDstStageMask{};
		std::vector<VkSemaphore> signalSemaphores{};
		std::vector<VkCommandBuffer> commandBuffers{};

		std::vector<VkSubmitInfo> infos;
		for (auto &&e : submitInfos)
		{
			waitSemaphores.clear();
			waitDstStageMask.clear();
			signalSemaphores.clear();
			commandBuffers.clear();
			for (auto &&a : e.waitSempahores)
			{
				waitSemaphores.emplace_back(static_cast<VKSemaphore *>(a)->GetHandle());
				waitDstStageMask.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
			}
			for (auto &&a : e.signalSempahores)
				signalSemaphores.emplace_back(static_cast<VKSemaphore *>(a)->GetHandle());
			for (auto &&a : e.commandBuffers)
				commandBuffers.emplace_back(static_cast<VKCommandBuffer *>(a)->GetHandle());

			infos.emplace_back(
				VkSubmitInfo{
					VK_STRUCTURE_TYPE_SUBMIT_INFO,
					nullptr,
					static_cast<uint32_t>(waitSemaphores.size()),
					waitSemaphores.data(),
					waitDstStageMask.data(),
					static_cast<uint32_t>(commandBuffers.size()),
					commandBuffers.data(),
					static_cast<uint32_t>(signalSemaphores.size()),
					signalSemaphores.data()});
		}
		if (VK_SUCCESS != vkQueueSubmit(mHandle, static_cast<uint32_t>(infos.size()), infos.data(), pFence ? static_cast<VKFence *>(pFence)->GetHandle() : VK_NULL_HANDLE))
			THROW("failed to submit command");
	}
	Result VKQueue::Present(const PresentInfo &presentInfo)
	{
		std::vector<VkSemaphore> waitSemaphores;
		for (auto &&a : presentInfo.waitSemaphores)
		{
			waitSemaphores.emplace_back(static_cast<VKSemaphore *>(a)->GetHandle());
		}
		std::vector<VkSwapchainKHR> swapchains;
		for (auto &&a : presentInfo.swapchains)
		{
			swapchains.emplace_back(static_cast<VKSwapchain*>(a)->GetHandle());
		}
		//std::vector<VkResult> results(swapchains.size());
		VkPresentInfoKHR info{
			VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			nullptr,
			static_cast<uint32_t>(waitSemaphores.size()),
			waitSemaphores.data(),
			static_cast<uint32_t>(swapchains.size()),
			swapchains.data(),
			presentInfo.imageIndices.data(),
		};
		//results.data()};
		auto res=vkQueuePresentKHR(mHandle, &info);

		switch (res)
		{
		case VK_SUCCESS:
		case VK_SUBOPTIMAL_KHR:
			return Result::SUCCESS;
		case VK_ERROR_OUT_OF_DATE_KHR:
			return Result::SHIT_ERROR_OUT_OF_DATE;
		default:
			return Result::SHIT_ERROR;
		}
	}
	void VKQueue::WaitIdle()
	{
		vkQueueWaitIdle(mHandle);
	}
} // namespace Shit
