/**
 * @file ShitDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitDevice.hpp"
#include "ShitCommandPool.hpp"
#include "ShitCommandBuffer.hpp"
#include "ShitQueue.hpp"

namespace Shit
{
	Device::Device(const DeviceCreateInfo &createInfo) : mCreateInfo(createInfo)
	{
	}
	void Device::CreateOneTimeCommandPool()
	{
		//create a transfer command pool for memory transfer operation
		auto transferQueueFamilyIndex = GetQueueFamilyIndexByFlag(
			QueueFlagBits::TRANSFER_BIT | QueueFlagBits::GRAPHICS_BIT | QueueFlagBits::COMPUTE_BIT,
			{});
		mpOneTimeCommandPool = Create(
			//{CommandPoolCreateFlagBits::TRANSIENT_BIT,
			{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
			 transferQueueFamilyIndex.value()});
		mpOneTimeCommandQueue = Create({transferQueueFamilyIndex->index, transferQueueFamilyIndex->count - 1});
	}
	void Device::Destroy(const Shader *pShader)
	{
		RemoveSmartPtrFromContainer(mShaders, pShader);
	}
	void Device::Destroy(const CommandPool *pCommandPool)
	{
		RemoveSmartPtrFromContainer(mCommandPools, pCommandPool);
	}
	void Device::Destroy(const Buffer *pBuffer)
	{
		RemoveSmartPtrFromContainer(mBuffers, pBuffer);
	}
	void Device::Destroy(const Image *pImage)
	{
		RemoveSmartPtrFromContainer(mImages, pImage);
	}
	void Device::Destroy(const ImageView *pImageView)
	{
		RemoveSmartPtrFromContainer(mImageViews, pImageView);
	}
	void Device::Destroy(const Semaphore *pSemaphore)
	{
		RemoveSmartPtrFromContainer(mSemaphores, pSemaphore);
	}
	void Device::Destroy(const Fence *pFence)
	{
		RemoveSmartPtrFromContainer(mFences, pFence);
	}
	void Device::Destroy(const DescriptorSetLayout *pSetLayout)
	{
		RemoveSmartPtrFromContainer(mDescriptorSetLayouts, pSetLayout);
	}
	void Device::Destroy(const PipelineLayout *pLayout)
	{
		RemoveSmartPtrFromContainer(mPipelineLayouts, pLayout);
	}
	void Device::Destroy(const RenderPass *pRenderPass)
	{
		RemoveSmartPtrFromContainer(mRenderPasses, pRenderPass);
	}
	void Device::Destroy(const Framebuffer *pFramebuffer)
	{
		RemoveSmartPtrFromContainer(mFramebuffers, pFramebuffer);
	}
	void Device::Destroy(const Swapchain *pSwapchain)
	{
		RemoveSmartPtrFromContainer(mSwapchains, pSwapchain);
	}
	void Device::Destroy(const Pipeline *pPipeline)
	{
		RemoveSmartPtrFromContainer(mPipelines, pPipeline);
	}
	void Device::Destroy(const DescriptorPool *pDescriptorPool)
	{
		RemoveSmartPtrFromContainer(mDescriptorPools, pDescriptorPool);
	}
	void Device::Destroy(const Sampler *pSampler)
	{
		RemoveSmartPtrFromContainer(mSamplers, pSampler);
	}
	void Device::ExecuteOneTimeCommands(const std::function<void(CommandBuffer *)> &func)
	{
		static CommandBuffer *pOneTimeCommandBuffer;
		if (!pOneTimeCommandBuffer)
		{
			std::vector<CommandBuffer *> cmdBuffers;
			if (cmdBuffers.empty())
				mpOneTimeCommandPool->CreateCommandBuffers({CommandBufferLevel::PRIMARY, 1}, cmdBuffers);
			pOneTimeCommandBuffer = cmdBuffers[0];
		}
		CommandBufferBeginInfo beginInfo{
			CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT};
		pOneTimeCommandBuffer->Begin(beginInfo);
		func(pOneTimeCommandBuffer);
		pOneTimeCommandBuffer->End();
		mpOneTimeCommandQueue->Submit({{{}, {pOneTimeCommandBuffer}}}, nullptr);
		mpOneTimeCommandQueue->WaitIdle();
		pOneTimeCommandBuffer->Reset(CommandBufferResetFlatBits::RELEASE_RESOURCES_BIT);
	}
} // namespace Shit
