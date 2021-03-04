/**
 * @file VKRenderSystem.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.h>
#include "VKPrerequisites.h"
#include "VKSwapchain.h"
#include "VKShader.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKQueue.h"

namespace Shit
{
	class VKRenderSystem final : public RenderSystem
	{
		std::vector<VkLayerProperties> mInstanceLayerProperties;
		std::vector<WindowAttribute> mWindowAttributes;

	private:
		bool CheckLayerSupport(const char *layerName);

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		void CreateSurface(ShitWindow *pWindow) override;

		decltype(auto) GetWindowAttributeIterator(const ShitWindow *pWindow)
		{
			auto end = mWindowAttributes.end();
			for (auto it = mWindowAttributes.begin(); it != end; ++it)
				if (it->pWindow == pWindow)
					return it;
			return end;
		}

		void ProcessWindowEvent(const Event &ev) override;

	public:
		VKRenderSystem(const RenderSystemCreateInfo &createInfo);

		~VKRenderSystem() override
		{
		}

		Device *CreateDevice(const DeviceCreateInfo& createInfo) override;

		Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) override;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;
		void DestroyShader(Shader* pShader) override;

		GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		CommandPool *CreateCommandPool(const CommandPoolCreateInfo &createInfo) override;
		void DestroyCommandPool(CommandPool *commandPool) override;

		CommandBuffer* CreateCommandBuffer(const CommandBufferCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Result WaitForFence(Device *pDevice, Fence *fence, uint64_t timeout) override;
	};
}