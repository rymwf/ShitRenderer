/**
 * @file VKRenderSystem.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.hpp>
#include "VKPrerequisites.hpp"
#include "VKSurface.hpp"
#include "VKDevice.hpp"

namespace Shit
{
	class VKRenderSystem final : public RenderSystem
	{
		std::vector<VkLayerProperties> mInstanceLayerProperties;

	private:
		bool CheckLayerSupport(const char *layerName);

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		std::unique_ptr<Surface> CreateSurface([[maybe_unused]] const SurfaceCreateInfo &createInfo, ShitWindow *pWindow) override;

		void LoadInstantceExtensionFunctions();

	public:
		VKRenderSystem(const RenderSystemCreateInfo &createInfo);

		~VKRenderSystem() override;

		Device *CreateDevice(const DeviceCreateInfo &createInfo) override;

		PFN_vkVoidFunction GetInstanceProcAddr(const char *pName);
	};
}