/**
 * @file ShitDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitSwapchain.h"
#include "ShitShader.h"
#include "ShitPipeline.h"
#include "ShitCommandPool.h"
#include "ShitCommandBuffer.h"
#include "ShitQueue.h"
#include "ShitSurface.h"
#include "ShitBuffer.h"
#include "ShitImage.h"
#include "ShitSampler.h"
#include "ShitDescriptor.h"
#include "ShitRenderPass.h"
#include "ShitFramebuffer.h"
#include "ShitSemaphore.h"
#include "ShitFence.h"

namespace Shit
{

	class Device
	{
	protected:
		std::list<std::unique_ptr<Queue>> mQueues;
		std::list<std::unique_ptr<CommandPool>> mCommandPools;
		std::list<std::unique_ptr<Pipeline>> mPipelines;
		std::list<std::unique_ptr<Shader>> mShaders;
		std::list<std::unique_ptr<Buffer>> mBuffers;
		std::list<std::unique_ptr<Image>> mImages;
		std::list<std::unique_ptr<ImageView>> mImageViews;
		std::list<std::unique_ptr<DescriptorSetLayout>> mDescriptorSetLayouts;
		std::list<std::unique_ptr<DescriptorSet>> mDescriptorSets;
		std::list<std::unique_ptr<PipelineLayout>> mPipelineLayouts;
		std::list<std::unique_ptr<RenderPass>> mRenderPasses;
		std::list<std::unique_ptr<Framebuffer>> mFramebuffers;
		std::list<std::unique_ptr<Semaphore>> mSemaphores;
		std::list<std::unique_ptr<Fence>> mFences;
		std::list<std::unique_ptr<Swapchain>> mSwapchains;

		CommandPool *mpOneTimeCommandPool;
		Queue *mpOneTimeCommandQueue;

		DeviceCreateInfo mCreateInfo;

	public:
		Device(const DeviceCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Device() {}

		constexpr const DeviceCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}

		virtual std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex([[maybe_unused]] ShitWindow *window)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, 1}};
		}
		virtual std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(
			[[maybe_unused]] QueueFlagBits flags, [[maybe_unused]] const std::unordered_set<uint32_t> &skipIndices)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, 1}};
		}

		virtual void GetWindowPixelFormats(const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats) = 0;
		virtual void GetPresentModes(const ShitWindow *pWindow, std::vector<PresentMode> &presentModes) = 0;

		/**
		 * @brief when creating a swapchain for a new window, user should create a new device  
		 * 
		 * @param createInfo 
		 * @param pWindow 
		 * @return Swapchain* 
		 */
		virtual Swapchain *Create(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) = 0;

		virtual Shader *Create(const ShaderCreateInfo &createInfo) = 0;
		virtual Pipeline *Create(const GraphicsPipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *Create(const CommandPoolCreateInfo &createInfo) = 0;

		/**
		 * @brief Create a Device Queue object
		 * 
		 * @param createInfo only device is useful for opengl
		 * @return Queue* 
		 */
		virtual Queue *Create(const QueueCreateInfo &createInfo) = 0;

		virtual Buffer *Create(const BufferCreateInfo &createInfo, void *pData) = 0;
		virtual Image *Create(const ImageCreateInfo &createInfo, void *pData) = 0;
		virtual ImageView *Create(const ImageViewCreateInfo &createInfo) = 0;
		virtual DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) = 0;
		virtual PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) = 0;
		virtual RenderPass *Create(const RenderPassCreateInfo &createInfo) = 0;
		virtual Framebuffer *Create(const FramebufferCreateInfo &createInfo) = 0;
		virtual Semaphore *Create(const SemaphoreCreateInfo &createInfo) = 0;
		virtual Fence *Create(const FenceCreateInfo &createInfo) = 0;

		void Destroy(const Swapchain *pSwapchain);
		void Destroy(const Shader *pShader);
		void Destroy(const Pipeline *pPipeline);
		void Destroy(const DescriptorSet *pDescriptorSet);
		void Destroy(const CommandPool *commandPool);
		void Destroy(const Buffer *pBuffer);
		void Destroy(const Image *pImage);
		void Destroy(const ImageView *pImageView);
		void Destroy(const DescriptorSetLayout *pSetLayout);
		void Destroy(const PipelineLayout *pLayout);
		void Destroy(const RenderPass *pRenderPass);
		void Destroy(const Semaphore *pSemaphore);
		void Destroy(const Fence *pFence);
		void Destroy(const Framebuffer *pFramebuffer);
	};
}
