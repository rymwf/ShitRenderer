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
	Swapchain* VKDevice::CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		VkSurfaceKHR surface = static_cast<VKSurface *>(pWindow->GetSurface())->GetHandle();

		auto presentQueueFamilyIndex = GetPresentQueueFamilyIndex(pWindow);

		if (!presentQueueFamilyIndex.has_value())
			THROW("current device do not support present to surface");

		//set format
		std::vector<VkSurfaceFormatKHR> surfaceFormats;

		VK::querySurfaceFormats(mPhysicalDevice, surface, surfaceFormats);

		VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0]; //default rgba8unorm

		VkFormat format = Map(createInfo.format);
		VkColorSpaceKHR colorSpace = Map(createInfo.colorSpace);
		//change to srgba8
		for (auto &&e : surfaceFormats)
		{
			if (e.format == format && e.colorSpace == colorSpace)
			{
				surfaceFormat = e;
				break;
			}
		}

		std::vector<VkPresentModeKHR> presentModes;
		VK::querySurfacePresentModes(mPhysicalDevice, surface, presentModes);
		VkPresentModeKHR presentMode;
		auto dstmode = Map(createInfo.presentMode);
		for (auto &&e : presentModes)
		{
			if (dstmode == e)
			{
				presentMode = e;
				break;
			}
		}
		pWindow->SetSwapchain(std::make_shared<VKSwapchain>(mDevice, createInfo, surface, surfaceFormat, presentMode));
		return pWindow->GetSwapchain();
	}

	Shader *VKDevice::CreateShader(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<VKShader>(mDevice, createInfo));
		return mShaders.back().get();
	}
	void VKDevice::DestroyShader(Shader *pShader)
	{
		for (auto it = mShaders.begin(), end = mShaders.end(); it != end; ++it)
		{
			if (it->get() == pShader)
			{
				mShaders.erase(it);
				break;
			}
		}
	}
	GraphicsPipeline *VKDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
	{
		return nullptr;
	}
	CommandPool *VKDevice::CreateCommandPool(const CommandPoolCreateInfo &createInfo)
	{
		mCommandPools.emplace_back(std::make_unique<VKCommandPool>(mDevice, createInfo));
		return mCommandPools.back().get();
	}
	void VKDevice::DestroyCommandPool(CommandPool *commandPool)
	{
		for (auto it = mCommandPools.begin(), end = mCommandPools.end(); it != end; ++it)
		{
			if (it->get() == commandPool)
			{
				mCommandPools.erase(it);
				break;
			}
		}
	}
	CommandBuffer *VKDevice::CreateCommandBuffer(const CommandBufferCreateInfo &createInfo)
	{
		mCommandBuffers.emplace_back(std::make_unique<VKCommandBuffer>(mDevice, createInfo));
		return mCommandBuffers.back().get();
	}
	Queue *VKDevice::CreateDeviceQueue(const QueueCreateInfo &createInfo)
	{
		mQueues.emplace_back(std::make_unique<VKQueue>(mDevice, createInfo));
		return mQueues.back().get();
	}
	Result VKDevice::WaitForFence(Fence *fence, uint64_t timeout)
	{
		return Result::SUCCESS;
	}
	Buffer *VKDevice::CreateBuffer(const BufferCreateInfo &createInfo, void *pData)
	{
		mBuffers.emplace_back(std::make_unique<VKBuffer>(mDevice, mPhysicalDevice, createInfo, pData));
		return mBuffers.back().get();
	}
} // namespace Shit
