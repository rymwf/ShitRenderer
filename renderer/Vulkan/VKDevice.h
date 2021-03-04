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

		std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex(ShitWindow *pWindow) override;

		VkPhysicalDevice GetPhysicalDevice()const
		{
			return mPhysicalDevice;
		}

		std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices) override;
	};

} // namespace Shit
