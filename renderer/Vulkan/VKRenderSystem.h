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
#include "VKSurface.h"
#include "VKDevice.h"

namespace Shit
{
	class VKRenderSystem final : public RenderSystem
	{
		std::vector<VkLayerProperties> mInstanceLayerProperties;

	private:
		bool CheckLayerSupport(const char *layerName);

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		std::shared_ptr<Surface> CreateSurface([[maybe_unused]] const SurfaceCreateInfo &createInfo, ShitWindow *pWindow) override;

	public:
		VKRenderSystem(const RenderSystemCreateInfo &createInfo);

		~VKRenderSystem() override
		{
			mWindows.clear();
			VkDevice device;
			for (int i = mDevices.size() - 1; i >= 0; --i)
			{
				device = static_cast<VKDevice *>(mDevices[i].get())->GetHandle();
				mDevices.erase(mDevices.begin() + i);
				vkDestroyDevice(device, nullptr);
			}
		}

		Device *CreateDevice(const DeviceCreateInfo &createInfo) override;
	};
}