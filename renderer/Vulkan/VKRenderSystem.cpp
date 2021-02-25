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

namespace Shit
{

	extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(const Shit::RenderSystemCreateInfo &createInfo)
	{
		return new VKRenderSystem(createInfo);
	}
	extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem)
	{
		delete pRenderSystem;
	}

	void VKRenderSystem::QueryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties)
	{
		uint32_t count;
		vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
		extensionProperties.resize(count);
		vkEnumerateInstanceExtensionProperties(layerName, &count, extensionProperties.data());
	}
	void VKRenderSystem::QueryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties)
	{
		uint32_t count;
		vkEnumerateInstanceLayerProperties(&count, nullptr);
		layerProperties.resize(count);
		vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
	}

	bool VKRenderSystem::CheckLayerSupport(const char *layerName)
	{
		if (mInstanceLayerProperties.empty())
		{
			QueryInstanceLayerProperties(mInstanceLayerProperties);
#ifndef NDEBUG
			for (auto &&layer : mInstanceLayerProperties)
			{
				LOG("============================================");
				LOG_VAR(layer.layerName);
				LOG_VAR(layer.specVersion);
				LOG_VAR(layer.implementationVersion);
				LOG_VAR(layer.description);

				std::vector<VkExtensionProperties> layerExtensionProperties;
				QueryInstanceExtensionProperties(layer.layerName, layerExtensionProperties);
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
		QueryInstanceExtensionProperties(nullptr, instanceExtensionProperties);
		std::vector<const char *> extensionNames;
		extensionNames.reserve(instanceExtensionProperties.size());
		LOG("vulkan implementaion extenions:");
		for (auto &e : instanceExtensionProperties)
		{
			extensionNames.emplace_back(e.extensionName);
			LOG_VAR(e.extensionName);
		}

		std::vector<const char *> layers;

		if (createInfo.debug)
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
		if (vkCreateInstance(&instanceInfo, 0, &mInstance) != VK_SUCCESS)
			throw std::runtime_error("create instance failed");
	}

	Context *VKRenderSystem::CreateContext(const ContextCreateInfo &createInfo)
	{
		for (auto &&windowSurface : mWindowSurfaces)
		{
			if (windowSurface.window.get() == createInfo.pWindow)
			{
#ifdef _WIN32
				windowSurface.surface = std::move(std::make_unique<VKContext>(mInstance, createInfo));
				return windowSurface.surface.get();
#endif
			}
		}
		throw std::runtime_error("failed to create surface");
	}
}