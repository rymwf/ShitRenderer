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

namespace Shit
{
	class VKRenderSystem final : public RenderSystem
	{
		std::vector<VkLayerProperties> mInstanceLayerProperties;

		std::vector<WindowAttribute> mWindowAttributes;

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

		void DeleteSurface(const ShitWindow *pWindow)
		{
			vkDestroySurfaceKHR(vk_instance, GetWindowAttributeIterator(pWindow)->surface, nullptr);
		}

		void ProcessWindowEvent(const Event &ev) override;

	public:
		VKRenderSystem(const RenderSystemCreateInfo &createInfo);

		~VKRenderSystem() override
		{
			for (auto &&e : mWindowAttributes)
				vkDestroySurfaceKHR(vk_instance, e.surface, nullptr);
			mDevices.clear();
			vkDestroyInstance(vk_instance, nullptr);
		}

		Device *CreateDevice(PhysicalDevice *pPhyicalDevice, ShitWindow *pWindow) override;

		Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) override;
	};
}