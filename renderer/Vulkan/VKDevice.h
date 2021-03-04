/**
 * @file VKDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDevice.h>
#include "VKPrerequisites.h"

namespace Shit
{

	class VKDevice final:public Device
	{
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;

		std::optional<uint32_t> mGraphicQueueFamilyIndex;
		std::optional<uint32_t> mTransferQueueFamilyIndex;

	public:
		VKDevice(PhysicalDevice physicalDevice);
		~VKDevice() override
		{
			vkDestroyDevice(mDevice, nullptr);
		}
		VkDevice GetHandle()
		{
			return mDevice;
		}

		std::optional<uint32_t> GetPresentQueueFamilyIndex(VkSurfaceKHR surface);

		VkPhysicalDevice GetPhysicalDevice()const
		{
			return mPhysicalDevice;
		}

		std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(VkQueueFlags flag, const std::vector<uint32_t> &skipIndices);
	};

} // namespace Shit
