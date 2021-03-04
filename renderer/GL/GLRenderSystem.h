/**
 * @file GLRenderSystem.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.h>

#include "GLPrerequisites.h"
#include "GLDevice.h"
#include "GLSwapchain.h"
#include "GLShader.h"
#include "GLPipeline.h"
#include "GLCommandBuffer.h"

namespace Shit
{

	class GLRenderSystem final : public RenderSystem
	{

		void ProcessWindowEvent(const Event &ev) override;

	public:
		GLRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo)
		{
		}
		~GLRenderSystem() override
		{
		}

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		Device *CreateDevice(const DeviceCreateInfo &createInfo) override
		{
#ifdef _WIN32
			mDevices.emplace_back(std::make_unique<GLDeviceWin32>(createInfo.pWindow));
#else
			static_assert(0, "GL CreateDevice is not implemented yet");
#endif
			return mDevices.back().get();
		};
		Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) override;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;
		void DestroyShader(Shader *pShader) override;

		GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		CommandBuffer *CreateCommandBuffer(const CommandBufferCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Result WaitForFence(Device *pDevice, Fence *fence, uint64_t timeout) override;
	};
}