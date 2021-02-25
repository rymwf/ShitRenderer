/**
 * @file VKRenderSystem.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.h>

#include "VKPrerequisites.h"
#include "VKContext.h"
namespace Shit
{

	class VKRenderSystem final : public RenderSystem
	{
		VkInstance mInstance;

		void QueryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties);
		void QueryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties);

		bool CheckLayerSupport(const char *layerName);

		std::vector<VkLayerProperties> mInstanceLayerProperties;

	public:
		VKRenderSystem(const RenderSystemCreateInfo &createInfo);

		~VKRenderSystem() override
		{
			vkDestroyInstance(mInstance, nullptr);
		}

		Context *CreateContext(const ContextCreateInfo &createInfo) override;

	};
}