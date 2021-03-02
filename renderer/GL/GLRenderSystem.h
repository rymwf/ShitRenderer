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

namespace Shit
{

	class GLRenderSystem final : public RenderSystem
	{

		void ProcessWindowEvent(const Event &ev) override;

	public:
		std::vector<std::unique_ptr<Swapchain>> mSwapchains;

		GLRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo)
		{
		}
		~GLRenderSystem() override
		{
		}

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		Device *CreateDevice([[maybe_unused]] PhysicalDevice *pPhyicalDevice, ShitWindow *pWindow) override
		{
#ifdef _WIN32
			mDevices.emplace_back(std::make_unique<GLDeviceWin32>(pWindow));
#else
			static_assert(0, "GL CreateDevice is not implemented yet");
#endif
			return mDevices.back().get();
		};
		Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) override;
	};
}