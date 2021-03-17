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
		std::vector<std::unique_ptr<Queue>> mQueues;
		std::vector<std::unique_ptr<CommandPool>> mCommandPools;
		std::vector<std::unique_ptr<Pipeline>> mPipelines;
		std::vector<std::unique_ptr<Shader>> mShaders;
		std::vector<std::unique_ptr<Buffer>> mBuffers;
		std::vector<std::unique_ptr<Image>> mImages;
		std::vector<std::unique_ptr<ImageView>> mImageViews;
		std::vector<std::unique_ptr<DescriptorSetLayout>> mDescriptorSetLayouts;
		std::vector<std::unique_ptr<DescriptorSet>> mDescriptorSets;
		std::vector<std::unique_ptr<PipelineLayout>> mPipelineLayouts;
		std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
		std::vector<std::unique_ptr<Framebuffer>> mFramebuffers;
		std::vector<std::unique_ptr<Semaphore>> mSemaphores;
		std::vector<std::unique_ptr<Fence>> mFences;
		std::vector<std::unique_ptr<Swapchain>> mSwapchains;

		CommandPool *mpOneTimeCommandPool;
		Queue *mpOneTimeCommandQueue;

	public:
		virtual ~Device() {}
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

		virtual Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) = 0;

		virtual Shader *CreateShader(const ShaderCreateInfo &createInfo) = 0;
		virtual Pipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *CreateCommandPool(const CommandPoolCreateInfo &createInfo) = 0;

		/**
		 * @brief Create a Device Queue object
		 * 
		 * @param createInfo only device is useful for opengl
		 * @return Queue* 
		 */
		virtual Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) = 0;

		virtual Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) = 0;
		virtual Image *CreateImage(const ImageCreateInfo &createInfo, void *pData) = 0;
		virtual ImageView *CreateImageView(const ImageViewCreateInfo &createInfo) = 0;
		virtual DescriptorSetLayout *CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) = 0;
		virtual PipelineLayout *CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo) = 0;
		virtual RenderPass *CreateRenderPass(const RenderPassCreateInfo &createInfo) = 0;
		virtual Framebuffer *CreateFramebuffer(const FramebufferCreateInfo &createInfo) = 0;
		virtual Semaphore *CreateDeviceSemaphore(const SemaphoreCreateInfo &createInfo) = 0;
		virtual Fence *CreateFence(const FenceCreateInfo &createInfo) = 0;

		void Destroy(const Shader *pShader);
		void Destroy(const CommandPool *commandPool);
		void Destroy(const Buffer *pBuffer);
		void Destroy(const Image *pImage);
		void Destroy(const ImageView *pImageView);
		void Destroy(const DescriptorSetLayout *pSetLayout);
		void Destroy(const PipelineLayout *pLayout);
		void Destroy(const RenderPass *pRenderPass);
		void Destroy(const Semaphore *pSemaphore);
		void Destroy(const Fence *pFence);
		void Destroy(const Framebuffer* pFramebuffer);
	};
}
