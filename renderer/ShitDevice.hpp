/**
 * @file ShitDevice.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitSwapchain.hpp"
#include "ShitShader.hpp"
#include "ShitPipeline.hpp"
#include "ShitCommandPool.hpp"
#include "ShitCommandBuffer.hpp"
#include "ShitQueue.hpp"
#include "ShitSurface.hpp"
#include "ShitBuffer.hpp"
#include "ShitImage.hpp"
#include "ShitSampler.hpp"
#include "ShitDescriptor.hpp"
#include "ShitRenderPass.hpp"
#include "ShitFramebuffer.hpp"
#include "ShitSemaphore.hpp"
#include "ShitFence.hpp"

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
		std::list<std::unique_ptr<PipelineLayout>> mPipelineLayouts;
		std::list<std::unique_ptr<RenderPass>> mRenderPasses;
		std::list<std::unique_ptr<Framebuffer>> mFramebuffers;
		std::list<std::unique_ptr<Semaphore>> mSemaphores;
		std::list<std::unique_ptr<Fence>> mFences;
		std::list<std::unique_ptr<Swapchain>> mSwapchains;
		std::list<std::unique_ptr<Sampler>> mSamplers;
		std::list<std::unique_ptr<DescriptorPool>> mDescriptorPools;

		CommandPool *mpOneTimeCommandPool;
		Queue *mpOneTimeCommandQueue;

		DeviceCreateInfo mCreateInfo;

		void CreateOneTimeCommandPool();

	public:
		Device(const DeviceCreateInfo &createInfo);
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
		virtual Pipeline *Create(const ComputePipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *Create(const CommandPoolCreateInfo &createInfo) = 0;

		/**
		 * @brief Create a Device Queue object
		 * 
		 * @param createInfo only device is useful for opengl
		 * @return Queue* 
		 */
		virtual Queue *Create(const QueueCreateInfo &createInfo) = 0;

		virtual Buffer *Create(const BufferCreateInfo &createInfo, const void *pData) = 0;
		virtual Image *Create(const ImageCreateInfo &createInfo, const void *pData) = 0;
		virtual ImageView *Create(const ImageViewCreateInfo &createInfo) = 0;
		virtual DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) = 0;
		virtual PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) = 0;
		virtual RenderPass *Create(const RenderPassCreateInfo &createInfo) = 0;
		virtual Framebuffer *Create(const FramebufferCreateInfo &createInfo) = 0;
		virtual Semaphore *Create(const SemaphoreCreateInfo &createInfo) = 0;
		virtual Fence *Create(const FenceCreateInfo &createInfo) = 0;
		virtual Sampler *Create(const SamplerCreateInfo &createInfo) = 0;
		virtual DescriptorPool *Create(const DescriptorPoolCreateInfo &createInfo) = 0;
		virtual void UpdateDescriptorSets(const std::vector<WriteDescriptorSet> &descriptorWrites, const std::vector<CopyDescriptorSet> &descriptorCopies) = 0;

		void ExecuteOneTimeCommands(const std::function<void(CommandBuffer *)> &func);

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
		void Destroy(const DescriptorPool *pDescriptorPool);
		void Destroy(const Sampler *pSampler);
	};
}
