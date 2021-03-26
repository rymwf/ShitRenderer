/**
 * @file ShitDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitDevice.h"

namespace Shit
{
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
} // namespace Shit
