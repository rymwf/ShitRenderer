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
	}

	std::optional<uint32_t> VKDevice::GetPresentQueueFamilyIndex(VkSurfaceKHR surface)
	{
		return VK::findQueueFamilyIndexPresent(mPhysicalDevice, static_cast<uint32_t>(mQueueFamilyProperties.size()), surface);
	}

	std::optional<QueueFamilyIndex> VKDevice::GetQueueFamilyIndexByFlag(VkQueueFlags flag, const std::vector<uint32_t> &skipIndices)
	{
		auto index = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, VK_QUEUE_TRANSFER_BIT, {});
		if (index.has_value())
			return std::optional<QueueFamilyIndex>{{*index, mQueueFamilyProperties[*index].queueCount}};
		else
			return std::nullopt;
	}
} // namespace Shit
