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

#define DESTROY_OBJECT(containers, x)                 \
	for (int i = containers.size() - 1; i >= 0; --i)  \
	{                                                 \
		if (containers[i].get() == x)                 \
		{                                             \
			containers.erase(containers.begin() + i); \
			break;                                    \
		}                                             \
	}

namespace Shit
{
	void Device::Destroy(const Shader *pShader)
	{
		DESTROY_OBJECT(mShaders, pShader);
	}
	void Device::Destroy(const CommandPool *pCommandPool)
	{
		DESTROY_OBJECT(mCommandPools, pCommandPool);
	}
	void Device::Destroy(const Buffer *pBuffer)
	{
		DESTROY_OBJECT(mBuffers, pBuffer);
	}
	void Device::Destroy(const Image *pImage)
	{
		DESTROY_OBJECT(mImages, pImage);
	}
	void Device::Destroy(const ImageView *pImageView)
	{
		DESTROY_OBJECT(mImageViews, pImageView);
	}
	void Device::Destroy(const Semaphore *pSemaphore)
	{
		DESTROY_OBJECT(mSemaphores, pSemaphore);
	}
	void Device::Destroy(const Fence *pFence)
	{
		DESTROY_OBJECT(mFences, pFence);
	}
	void Device::Destroy(const DescriptorSetLayout *pSetLayout)
	{
		DESTROY_OBJECT(mDescriptorSetLayouts, pSetLayout);
	}
	void Device::Destroy(const PipelineLayout *pLayout)
	{
		DESTROY_OBJECT(mPipelineLayouts, pLayout);
	}
	void Device::Destroy(const RenderPass *pRenderPass)
	{
		DESTROY_OBJECT(mRenderPasses, pRenderPass);
	}
	void Device::Destroy(const Framebuffer *pFramebuffer)
	{
		DESTROY_OBJECT(mFramebuffers, pFramebuffer);
	}

} // namespace Shit
