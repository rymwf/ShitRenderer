/**
 * @file VKContext.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "VKPrerequisites.h"
#include <renderer/ShitContext.h>

namespace Shit
{
	class VKContext final : public Context
	{
		VkSurfaceKHR mSurface;
		VkPhysicalDevice mPhysicalDevice;
		VkDevice mDevice;

		int RateDeviceSuitability(VkPhysicalDevice device);
		VkPhysicalDevice SelectPhysicalDevice();

		void CreatePhysicalDevice();

	public:
		VKContext(const ContextCreateInfo &createInfo);
		~VKContext()
		{
			vkDestroySurfaceKHR(vk_instance, mSurface, nullptr);
		}
	};

} // namespace Shit
