/**
 * @file VKDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKDevice.h"

namespace Shit
{

	VKDevice::VKDevice(PhysicalDevice physicalDevice) : mPhysicalDevice(static_cast<VkPhysicalDevice>(physicalDevice))
	{
		VK::queryQueueFamilyProperties(mPhysicalDevice, mQueueFamilyProperties);

		std::vector<VkExtensionProperties> properties;
		VK::queryDeviceExtensionProperties(mPhysicalDevice, properties);

		std::vector<const char *> extensionNames;
		extensionNames.reserve(properties.size());
		for (auto &extensionProperty : properties)
		{
			LOG(extensionProperty.extensionName);
			LOG(extensionProperty.specVersion);
			extensionNames.emplace_back(extensionProperty.extensionName);
		}

		//physical device  features
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(mPhysicalDevice, &deviceFeatures);

		deviceFeatures.sampleRateShading = true;

		std::vector<VkDeviceQueueCreateInfo> queueInfos;
		std::vector<float> queuePriorities;
		for (uint32_t i = 0, len = mQueueFamilyProperties.size(); i < len; ++i)
		{
			queuePriorities.clear();
			//TODO: how to arrange queue priorities
			queuePriorities.resize(mQueueFamilyProperties[i].queueCount, 1.f);
			queueInfos.emplace_back(
				VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
										NULL,
										0,
										i,									  //queue family index
										mQueueFamilyProperties[i].queueCount, //queue count
										queuePriorities.data()});
		}

		VkDeviceCreateInfo deviceInfo{
			VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			NULL,
			0,
			static_cast<uint32_t>(queueInfos.size()),
			queueInfos.data(),
			0, //deprecated
			0, //deprecated
			extensionNames.size(),
			extensionNames.data(),
			&deviceFeatures};
		if (vkCreateDevice(mPhysicalDevice, &deviceInfo, nullptr, &mDevice) != VK_SUCCESS)
			THROW("create logical device failed");

		mGraphicQueueFamilyIndex = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, VK_QUEUE_GRAPHICS_BIT, {});

		uint32_t b{0xffff};
		if (mGraphicQueueFamilyIndex.has_value())
		{
			LOG_VAR(mGraphicQueueFamilyIndex.value());
			b = mGraphicQueueFamilyIndex.value();
		}
		else
			LOG("current device do not support graphic queue");

		mTransferQueueFamilyIndex = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, VK_QUEUE_TRANSFER_BIT, {b});
		if (mTransferQueueFamilyIndex.has_value())
			LOG_VAR(mTransferQueueFamilyIndex.value());
		else
			LOG("current device do not support a pure transfer queue");
	}

	std::optional<uint32_t> VKDevice::GetPresentQueueFamilyIndex(VkSurfaceKHR surface)
	{
		return VK::findQueueFamilyIndexPresent(mPhysicalDevice, static_cast<uint32_t>(mQueueFamilyProperties.size()), surface);
		//LOG("there it not surface or current device do not support present queue");
	}
} // namespace Shit
