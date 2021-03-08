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

namespace Shit
{

	class Device
	{
	protected:
		std::vector<std::unique_ptr<CommandPool>> mCommandPools;
		std::vector<std::unique_ptr<Queue>> mQueues;
		std::vector<std::unique_ptr<GraphicsPipeline>> mGraphicsPipelines;
		std::vector<std::unique_ptr<Shader>> mShaders;
		std::vector<std::unique_ptr<Buffer>> mBuffers;
		std::vector<std::unique_ptr<Image>> mImages;
		std::vector<std::unique_ptr<ImageView>> mImageViews;
		std::vector<std::unique_ptr<DescriptorSetLayout>> mDescriptorSetLayouts;
		std::vector<std::unique_ptr<DescriptorSet>> mDescriptorSets;
		std::vector<std::unique_ptr<PipelineLayout>> mPipelineLayouts;
		std::vector<std::unique_ptr<RenderPass>> mRenderPasses;
		std::vector<std::unique_ptr<Framebuffer>> mFramebuffer;

		CommandPool *mpOneTimeCommandPool;
		Queue *mpOneTimeCommandQueue;

	public:
		virtual ~Device() {}
		virtual std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex([[maybe_unused]] ShitWindow *window)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, INT_MAX}};
		}
		virtual std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(
			[[maybe_unused]] QueueFlagBits flags, [[maybe_unused]] const std::unordered_set<uint32_t> &skipIndices)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, INT_MAX}};
		}

		virtual Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) = 0;

		virtual Shader *CreateShader(const ShaderCreateInfo &createInfo) = 0;
		void DestroyShader(Shader *pShader)
		{
			for (int i = mShaders.size() - 1; i >= 0; --i)
			{
				if (mShaders[i].get() == pShader)
				{
					mShaders.erase(mShaders.begin() + i);
					break;
				}
			}
		}

		virtual GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *CreateCommandPool([[maybe_unused]] [[maybe_unused]] const CommandPoolCreateInfo &createInfo)
		{
			LOG("currrent renderer do not support commandpool");
			return nullptr;
		}
		void DestroyCommandPool([[maybe_unused]] CommandPool *commandPool)
		{
			for (int i = mCommandPools.size() - 1; i >= 0; --i)
			{
				if (mCommandPools[i].get() == commandPool)
				{
					mCommandPools.erase(mCommandPools.begin() + i);
					break;
				}
			}
		}

		/**
		 * @brief Create a Device Queue object
		 * 
		 * @param createInfo only device is useful for opengl
		 * @return Queue* 
		 */
		virtual Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) = 0;

		/**
		 * @brief 
		 * 
		 * @param pDevice 
		 * @param fence 
		 * @param timeout  nanoseconds
		 * @return Result 
		 */
		virtual Result WaitForFence(Fence *fence, uint64_t timeout) = 0;

		virtual Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) = 0;
		void DestroyBuffer(Buffer *pBuffer)
		{
			for (int i = mBuffers.size() - 1; i >= 0; --i)
			{
				if (mBuffers[i].get() == pBuffer)
				{
					mBuffers.erase(mBuffers.begin() + i);
					break;
				}
			}
		}

		virtual Image *CreateImage(const ImageCreateInfo &createInfo, void *pData) = 0;
		void DestroyImage(Image *pImage)
		{
			for (int i = mImages.size() - 1; i >= 0; --i)
			{
				if (mImages[i].get() == pImage)
				{
					mImages.erase(mImages.begin() + i);
					break;
				}
			}
		}
		virtual ImageView *CreateImageView(const ImageViewCreateInfo &createInfo)
		{
			return nullptr;
		}

		virtual DescriptorSetLayout *CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo)
		{
			return nullptr;
		}
		virtual PipelineLayout *CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
		{
			return nullptr;
		}
		virtual RenderPass *CreateRenderPass(const RenderPassCreateInfo &createInfo)
		{
			return nullptr;
		}
		virtual Framebuffer *CreateFramebuffer(const FramebufferCreateInfo &createInfo)
		{
			return nullptr;
		}
	};
}
