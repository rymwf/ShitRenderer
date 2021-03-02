/**
 * @file VKRenderSystem.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKRenderSystem.h"
#include "VKDevice.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{

	VkInstance vk_instance;

	extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(const Shit::RenderSystemCreateInfo &createInfo)
	{
		return new VKRenderSystem(createInfo);
	}
	extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem)
	{
		delete pRenderSystem;
	}

	bool VKRenderSystem::CheckLayerSupport(const char *layerName)
	{
		if (mInstanceLayerProperties.empty())
		{
			VK::queryInstanceLayerProperties(mInstanceLayerProperties);
#ifndef NDEBUG
			for (auto &&layer : mInstanceLayerProperties)
			{
				LOG("============================================");
				LOG_VAR(layer.layerName);
				LOG_VAR(layer.specVersion);
				LOG_VAR(layer.implementationVersion);
				LOG_VAR(layer.description);

				std::vector<VkExtensionProperties> layerExtensionProperties;
				VK::queryInstanceExtensionProperties(layer.layerName, layerExtensionProperties);
				for (auto &&layerExtensionProp : layerExtensionProperties)
				{
					LOG_VAR(layerExtensionProp.specVersion);
					LOG_VAR(layerExtensionProp.extensionName);
				}
			}
#endif
		}
		for (auto &&layerProp : mInstanceLayerProperties)
			if (strcmp(layerProp.layerName, layerName) == 0)
				return true;
		return false;
	}

	VKRenderSystem::VKRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo)
	{
		uint32_t a = static_cast<uint32_t>(createInfo.version) & 0xffff;
		uint32_t apiversion;
		if (a)
			apiversion = VK_MAKE_VERSION((a >> 2) & 1, (a >> 1) & 1, a & 1);
		else
			vkEnumerateInstanceVersion(&apiversion);

		LOG_VAR(apiversion);

		VkApplicationInfo appInfo{
			VK_STRUCTURE_TYPE_APPLICATION_INFO,
			NULL,
			"application name",
			VK_MAKE_VERSION(1, 0, 0), //integer, app version
			"engine name",
			VK_MAKE_VERSION(0, 0, 0), //engine version
			apiversion};

		//add extensions
		std::vector<VkExtensionProperties> instanceExtensionProperties;
		VK::queryInstanceExtensionProperties(nullptr, instanceExtensionProperties);
		std::vector<const char *> extensionNames;
		extensionNames.reserve(instanceExtensionProperties.size());
		LOG("vulkan instance extenions:");
		for (auto &e : instanceExtensionProperties)
		{
			extensionNames.emplace_back(e.extensionName);
			LOG_VAR(e.extensionName);
		}

		std::vector<const char *> layers;

		if (static_cast<bool>(createInfo.flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
		{
			//enable validation layer
			if (CheckLayerSupport(LAYER_VALIDATION_KHRONOS_validation))
			{
				layers.emplace_back(LAYER_VALIDATION_KHRONOS_validation);
				//add extensions
			}
			else
				LOG("validation layer is not supported");
		}

		VkInstanceCreateInfo instanceInfo{
			VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, //must
			NULL,
			0, //must
			&appInfo,
			layers.size(),
			layers.data(),
			extensionNames.size(),
			extensionNames.data()};
		if (vkCreateInstance(&instanceInfo, 0, &vk_instance) != VK_SUCCESS)
			THROW("create instance failed");
	}

	void VKRenderSystem::CreateSurface(ShitWindow *pWindow)
	{
#ifdef _WIN32
		VkWin32SurfaceCreateInfoKHR createInfo{
			VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
			nullptr,
			0,
			static_cast<WindowWin32 *>(pWindow)->GetInstance(),
			static_cast<WindowWin32 *>(pWindow)->GetHWND(),
		};
#else
		static_assert(0, "there is no VK surface implementation");
#endif
		VkSurfaceKHR surface;
		if (vkCreateWin32SurfaceKHR(vk_instance, &createInfo, nullptr, &surface) != VK_SUCCESS)
			THROW("failed to create VK surface");

		mWindowAttributes.emplace_back(WindowAttribute{pWindow, surface});
	}
	void VKRenderSystem::EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices)
	{
		VK::queryPhysicalDevices(vk_instance, physicalDevices);
	}

	Device *VKRenderSystem::CreateDevice([[maybe_unused]] PhysicalDevice *pPhyicalDevice, [[maybe_unused]] ShitWindow *pWindow)
	{
		static PhysicalDevice physicalDevice;
		physicalDevice = VK::pickPhysicalDevice(vk_instance);
		mDevices.emplace_back(std::make_unique<VKDevice>(physicalDevice));
		return mDevices.back().get();
	}

	Swapchain *VKRenderSystem::CreateSwapchain(const SwapchainCreateInfo &createInfo)
	{
		auto windowAttribIt = GetWindowAttributeIterator(createInfo.pWindow);
		windowAttribIt->swapchain = std::move(std::make_unique<VKSwapchain>(createInfo, windowAttribIt->surface));
		return windowAttribIt->swapchain.get();
	}

	void VKRenderSystem::ProcessWindowEvent(const Event &ev)
	{
		switch (ev.type)
		{
		case EventType::WINDOW_CLOSE:
			auto it = GetWindowAttributeIterator(ev.pWindow);
			auto surface = it->surface;
			mWindowAttributes.erase(it);
			vkDestroySurfaceKHR(vk_instance, surface, nullptr);
			break;
		}
	}
}