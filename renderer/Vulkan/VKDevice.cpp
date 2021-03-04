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
#include <renderer/ShitWindow.h>
#include "VKSurface.h"

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

	std::optional<QueueFamilyIndex> VKDevice::GetPresentQueueFamilyIndex(ShitWindow *pWindow)
	{
		auto index = VK::findQueueFamilyIndexPresent(
			mPhysicalDevice,
			static_cast<uint32_t>(mQueueFamilyProperties.size()),
			static_cast<VKSurface *>(pWindow->GetSurface())->GetHandle());
		if (index.has_value())
			return std::optional<QueueFamilyIndex>{{*index, mQueueFamilyProperties[*index].queueCount}};
		else
			return std::nullopt;
	}

	std::optional<QueueFamilyIndex> VKDevice::GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices)
	{
		auto index = VK::findQueueFamilyIndexByFlag(mQueueFamilyProperties, Map(flag), skipIndices);
		if (index.has_value())
			return std::optional<QueueFamilyIndex>{{*index, mQueueFamilyProperties[*index].queueCount}};
		else
			return std::nullopt;
	}
} // namespace Shit
