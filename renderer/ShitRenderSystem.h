/**
 * @file ShitRenderer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitNonCopyable.h"
#include "ShitWindow.h"
#include "ShitDevice.h"
#include "ShitSwapchain.h"
#include "ShitShader.h"
#include "ShitPipeline.h"
#include "ShitCommandPool.h"
#include "ShitCommandBuffer.h"
#include "ShitQueue.h"
#include "ShitSurface.h"

namespace Shit
{
	class RenderSystem : public NonCopyable
	{
	protected:
		RenderSystemCreateInfo mCreateInfo;

		std::vector<std::unique_ptr<ShitWindow>> mWindows;
		std::vector<std::unique_ptr<Shader>> mShaders;
		std::vector<std::unique_ptr<GraphicsPipeline>> mGraphicsPipelines;
		std::vector<std::unique_ptr<CommandPool>> mCommandPools;
		std::vector<std::unique_ptr<CommandBuffer>> mCommandBuffers;
		std::vector<std::unique_ptr<Queue>> mQueues;
		std::vector<std::unique_ptr<Device>> mDevices;

	protected:
		RenderSystem() {}

		void DestroyDevice(const Device *pDevice);

		virtual void ProcessWindowEvent(const Event &ev) = 0;

		virtual Surface *CreateSurface([[maybe_unused]] const SurfaceCreateInfo &createInfo)
		{
			return nullptr;
		};

	public:
		RenderSystem(const RenderSystemCreateInfo &createInfo)
			: mCreateInfo(createInfo)
		{
		}
		virtual ~RenderSystem()
		{
			mWindows.clear();
		}

		const RenderSystemCreateInfo *GetCreateInfo() const
		{
			return &mCreateInfo;
		}

		ShitWindow *CreateRenderWindow(const WindowCreateInfo &createInfo);

		/**
		 * @brief Create a Device object, 
		 * create a device include all queue families supported by the physical device
		 * currently physical device selection is not supported, the method will use the current gpu(opengl) or the best gpu(Vulkan)
		 * 
		 * @param createInfo 
		 * @return Device* 
		 */
		virtual Device *CreateDevice(const DeviceCreateInfo& createInfo) = 0;

		/**
		 * @brief TODO: physical device not finished
		 * 
		 * @param physicalDevices 
		 */
		virtual void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) = 0;

		/**
		 * @brief Create a Swapchain object,
		 *  for opengl, just create a render context
		 * 
		 * @param createInfo 
		 * @return Swapchain* 
		 */
		virtual Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) = 0;

		virtual Shader *CreateShader(const ShaderCreateInfo &createInfo) = 0;
		virtual void DestroyShader(Shader *pShader) = 0;

		virtual GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) = 0;

		virtual CommandPool *CreateCommandPool([[maybe_unused]] const CommandPoolCreateInfo &createInfo)
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
		virtual Result WaitForFence(Device *pDevice, Fence *fence, uint64_t timeout) = 0;
	};

	SHIT_API RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
} // namespace Shit
