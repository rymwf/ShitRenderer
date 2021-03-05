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

namespace Shit
{

	class Device
	{
	protected:

		std::vector<std::unique_ptr<Shader>> mShaders;
		std::vector<std::unique_ptr<GraphicsPipeline>> mGraphicsPipelines;
		std::vector<std::unique_ptr<CommandPool>> mCommandPools;
		std::vector<std::unique_ptr<CommandBuffer>> mCommandBuffers;
		std::vector<std::unique_ptr<Queue>> mQueues;
		std::vector<std::unique_ptr<Buffer>> mBuffers;
		std::vector<std::unique_ptr<Image>> mImages;


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

		virtual Swapchain* CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) = 0;

		virtual Shader *CreateShader(const ShaderCreateInfo &createInfo) = 0;
		virtual void DestroyShader(Shader *pShader) = 0;

		virtual GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *CreateCommandPool([[maybe_unused]] [[maybe_unused]] const CommandPoolCreateInfo &createInfo)
		{
			LOG("currrent renderer do not support commandpool");
			return nullptr;
		}
		virtual void DestroyCommandPool([[maybe_unused]] CommandPool *commandPool) {}

		virtual CommandBuffer *CreateCommandBuffer(const CommandBufferCreateInfo &createInfo) = 0;

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
	};
}
