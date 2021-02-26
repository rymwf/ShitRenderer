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
#include <algorithm>

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

	Context *VKRenderSystem::CreateContext(const ContextCreateInfo &createInfo)
	{
		//TODO:fix this, let user choose physical device
		//select physical device
		ContextCreateInfo tempCreateInfo = createInfo;
		tempCreateInfo.phyicalDevice = SelectPhysicalDevice();

		for (auto &&windowContext : mWindowContexts)
		{
			if (windowContext.window.get() == createInfo.pWindow)
			{
#ifdef _WIN32
				//windowContext.surface = std::move(std::make_unique<VKContext>(createInfo));
				windowContext.context= std::move(std::make_unique<VKContext>(tempCreateInfo));
				return windowContext.context.get();
#endif
			}
		}
		THROW("failed to create surface");
	}
	void VKRenderSystem::EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices)
	{
		uint32_t count{};
		vkEnumeratePhysicalDevices(vk_instance, &count, nullptr);
		std::vector<VkPhysicalDevice> devices(count);
		vkEnumeratePhysicalDevices(vk_instance, &count, reinterpret_cast<VkPhysicalDevice *>(physicalDevices.data()));
	}

	int VKRenderSystem::RateDeviceSuitability(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		LOG_VAR(deviceProperties.apiVersion);
		LOG_VAR(VK_VERSION_MAJOR(deviceProperties.apiVersion));
		LOG_VAR(VK_VERSION_MINOR(deviceProperties.apiVersion));
		LOG_VAR(VK_VERSION_PATCH(deviceProperties.apiVersion));
		LOG_VAR(deviceProperties.driverVersion);
		LOG_VAR(deviceProperties.vendorID);
		LOG_VAR(deviceProperties.deviceID);
		LOG_VAR(deviceProperties.deviceType);
		LOG_VAR(deviceProperties.deviceName);
		LOG_VAR(deviceProperties.pipelineCacheUUID);
		LOG_VAR(deviceProperties.limits.maxVertexInputBindings);
		LOG_VAR(deviceProperties.limits.maxVertexInputBindingStride);
		LOG_VAR(deviceProperties.limits.maxVertexInputAttributes);
		LOG_VAR(deviceProperties.limits.maxImageDimension2D);
		// LOG_VAR(deviceProperties.sparseProperties);

		LOG_VAR(deviceFeatures.geometryShader);
		LOG_VAR(deviceFeatures.samplerAnisotropy);
		// Application can't function without geometry shaders
		int score = 0;
		if (!deviceFeatures.geometryShader)
		{
			LOG_VAR(score);
			LOG_VAR(deviceProperties.deviceName);
			return 0;
		}

		//discrete gpu is prefered
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			score += 1000;
		// Maximum possible size of textures affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		LOG_VAR(score);
		LOG_VAR(deviceProperties.deviceName);
		return score;
	}

	VkPhysicalDevice VKRenderSystem::SelectPhysicalDevice()
	{
		uint32_t physicalDeviceCount;
		vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, nullptr);
		LOG(physicalDeviceCount);
		if (physicalDeviceCount == 0)
		{
			THROW("failed to find GPUs with vulkan support");
		}
		//first element is score
		std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
		vkEnumeratePhysicalDevices(vk_instance, &physicalDeviceCount, devices.data());

		std::vector<std::pair<int, VkPhysicalDevice>> scoreddevices;
		for (auto &device : devices)
		{
			scoreddevices.emplace_back(RateDeviceSuitability(device), device);
		}
		std::sort(scoreddevices.begin(), scoreddevices.end());

		auto &temp = scoreddevices.back();
		if (temp.first == 0)
			THROW("failed to find a suitable GPU");

		return temp.second;
	}

}