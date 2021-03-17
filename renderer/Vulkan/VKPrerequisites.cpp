/**
 * @file VKPrerequisites.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKPrerequisites.h"
namespace Shit
{
	namespace VK
	{
		int rateDeviceSuitability(VkPhysicalDevice device)
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
//			LOG_VAR(deviceProperties.pipelineCacheUUID);
			LOG_VAR(deviceProperties.limits.maxVertexInputBindings);
			LOG_VAR(deviceProperties.limits.maxVertexInputBindingStride);
			LOG_VAR(deviceProperties.limits.maxVertexInputAttributes);
			LOG_VAR(deviceProperties.limits.maxImageDimension2D);
			//       LOG_VAR(deviceProperties.sparseProperties);

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

		void queryQueueFamilyProperties(VkPhysicalDevice physicalDevice, std::vector<VkQueueFamilyProperties> &queueFamilyProperties)
		{
			uint32_t queueFamilyPropertyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
			queueFamilyProperties.resize(queueFamilyPropertyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

#ifndef NDEBUG
			LOG_VAR(queueFamilyPropertyCount);
			for (uint32_t i = 0; i < queueFamilyPropertyCount; ++i)
			{
				LOG_VAR(i);
				LOG_VAR(queueFamilyProperties[i].queueFlags);
				LOG_VAR(queueFamilyProperties[i].queueCount);
				LOG_VAR(queueFamilyProperties[i].timestampValidBits);
				LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.width);
				LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.height);
				LOG_VAR(queueFamilyProperties[i].minImageTransferGranularity.depth);
			}
#endif
		}

		std::optional<uint32_t> findQueueFamilyIndexByFlag(std::vector<VkQueueFamilyProperties> &queueFamilyProperties, VkQueueFlags flag, const std::unordered_set<uint32_t> &skipIndices)
		{
			for (uint32_t i = 0, l = static_cast<uint32_t>(queueFamilyProperties.size()); i < l; ++i)
			{
				if (skipIndices.find(i) != skipIndices.end())
					continue;
				if (queueFamilyProperties[i].queueFlags & flag)
					return std::optional<uint32_t>(i);
			}
			return std::nullopt;
		}
		std::optional<uint32_t> findQueueFamilyIndexPresent(VkPhysicalDevice physicalDevice, uint32_t familyNum, VkSurfaceKHR surface)
		{
			VkBool32 surfaceSupported = false;
			for (uint32_t i = 0; i < familyNum && !surfaceSupported; ++i)
			{
				vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupported);
				if (surfaceSupported)
					return std::optional<uint32_t>(i);
			}
			return std::nullopt;
		}
		VkPhysicalDevice pickPhysicalDevice(VkInstance instance)
		{
			uint32_t physicalDeviceCount;
			vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
			LOG_VAR(physicalDeviceCount);
			if (physicalDeviceCount == 0)
			{
				THROW("failed to find GPUs with vulkan support");
			}
			//first element is score
			std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
			vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, devices.data());

			std::vector<std::pair<int, VkPhysicalDevice>> scoreddevices;
			for (auto &device : devices)
			{
				scoreddevices.emplace_back(rateDeviceSuitability(device), device);
			}
			std::sort(scoreddevices.begin(), scoreddevices.end());

			auto &temp = scoreddevices.back();
			if (temp.first == 0)
				THROW("failed to find a suitable GPU");

			return temp.second;
		}
		void queryPhysicalDeviceGroupInfo(VkInstance instance, std::vector<VkPhysicalDeviceGroupProperties> &physicalDeviceGroupProperties)
		{
			uint32_t physicalDeviceGroupCount;
			vkEnumeratePhysicalDeviceGroups(instance, &physicalDeviceGroupCount, nullptr);
			physicalDeviceGroupProperties.resize(physicalDeviceGroupCount);
			vkEnumeratePhysicalDeviceGroups(instance, &physicalDeviceGroupCount, physicalDeviceGroupProperties.data());
			LOG_VAR(physicalDeviceGroupCount);
			for (auto physicalDeviceGroupProperty : physicalDeviceGroupProperties)
			{
				LOG_VAR(physicalDeviceGroupProperty.physicalDeviceCount);
			}
		}

		void querySurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &surfaceFormats)
		{
			uint32_t surfaceFormatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
			surfaceFormats.resize(surfaceFormatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());
#ifndef NDEBUG
			LOG_VAR(surfaceFormatCount);
			for (auto &surfaceFormat : surfaceFormats)
			{
				LOG_VAR(surfaceFormat.colorSpace);
				LOG_VAR(surfaceFormat.format);
			}
#endif
		}
		void querySurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &presentModes)
		{
			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
			presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
#ifndef NDEBUG
			LOG_VAR(presentModeCount);
			for (auto &presentMode : presentModes)
				LOG_VAR(presentMode);
#endif
		}
		VkShaderModule createShaderModule(VkDevice logicalDevice, const std::string &code)
		{
			VkShaderModule ret{};
			VkShaderModuleCreateInfo createInfo{
				VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
				nullptr,
				0,
				code.size(),
				reinterpret_cast<const uint32_t *>(code.data())};

			if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed to create shadermodule");
			return ret;
		}
		VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayoutBinding> &setLayoutBindings)
		{
			VkDescriptorSetLayout ret;
			VkDescriptorSetLayoutCreateInfo createInfo{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				nullptr,
				0,
				static_cast<uint32_t>(setLayoutBindings.size()),
				setLayoutBindings.data()};

			if (vkCreateDescriptorSetLayout(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failded to create decriptor set layout");
			return ret;
		}

		VkPipelineLayout createPipelineLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayout> &setLayouts, const std::vector<VkPushConstantRange> &pushConstantRanges)
		{
			VkPipelineLayout ret;
			VkPipelineLayoutCreateInfo createInfo{
				VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				nullptr,
				0,
				static_cast<uint32_t>(setLayouts.size()),
				setLayouts.data(),
				static_cast<uint32_t>(pushConstantRanges.size()),
				pushConstantRanges.data()};

			if (vkCreatePipelineLayout(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed to create pipelinelayout");
			return ret;
		}
		VkCommandPool createCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex)
		{
			VkCommandPool ret;

			VkCommandPoolCreateInfo createInfo{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				nullptr,
				0,
				queueFamilyIndex};
			if (vkCreateCommandPool(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed to create command pool");
			return ret;
		}
		VkSemaphore createSemaphore(VkDevice logicalDevice)
		{
			VkSemaphore ret;
			VkSemaphoreCreateInfo createInfo{
				VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
			};
			if (vkCreateSemaphore(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed create semaphore");
			return ret;
		}

		void queryDisplayProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPropertiesKHR> &displayProperties)
		{
			uint32_t count;
			vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &count, nullptr);
			displayProperties.resize(count);
			vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &count, displayProperties.data());
#ifndef NDEBUG
			for (auto &displayProperty : displayProperties)
			{
				LOG_VAR(displayProperty.display);
				LOG_VAR(displayProperty.displayName);
				LOG_VAR(displayProperty.persistentContent);
				LOG_VAR(displayProperty.physicalDimensions.width);
				LOG_VAR(displayProperty.physicalDimensions.height);
				LOG_VAR(displayProperty.physicalResolution.width);
				LOG_VAR(displayProperty.physicalResolution.height);
				LOG_VAR(displayProperty.planeReorderPossible);
				LOG_VAR(displayProperty.supportedTransforms);
			}
#endif
		}
		void queryDisplayPlaneProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPlanePropertiesKHR> &displayPlaneProperties)
		{
			uint32_t count;
			vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &count, nullptr);
			displayPlaneProperties.resize(count);
			vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &count, displayPlaneProperties.data());
#ifndef NDEBUG
			for (auto displayPlaneProperty : displayPlaneProperties)
			{
				LOG_VAR(displayPlaneProperty.currentDisplay);
				LOG_VAR(displayPlaneProperty.currentStackIndex);
			}
#endif
		}
		void queryDisplayModeProperties(VkPhysicalDevice physicalDevice, VkDisplayKHR display, std::vector<VkDisplayModePropertiesKHR> &displayModeProperties)
		{
			uint32_t count;
			vkGetDisplayModePropertiesKHR(physicalDevice, display, &count, nullptr);
			displayModeProperties.resize(count);
			vkGetDisplayModePropertiesKHR(physicalDevice, display, &count, displayModeProperties.data());
#ifndef NDEBUG
			for (auto displayModeProperty : displayModeProperties)
			{
				LOG_VAR(displayModeProperty.displayMode);
				LOG_VAR(displayModeProperty.parameters.refreshRate);
				LOG_VAR(displayModeProperty.parameters.visibleRegion.width);
				LOG_VAR(displayModeProperty.parameters.visibleRegion.height);
			}
#endif
		}
		VkFence createFence(VkDevice logicalDevice, VkFenceCreateFlags flags)
		{
			VkFence ret;
			VkFenceCreateInfo createInfo{
				VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				nullptr,
				flags};
			if (vkCreateFence(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed to create fence");
			return ret;
		}
		uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeIndexFilter, VkMemoryPropertyFlags properties)
		{
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
#if 0
    LOG_VAR(memoryProperties.memoryHeapCount);
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
    {
        LOG_VAR(i);
        LOG_VAR(memoryProperties.memoryHeaps[i].flags);
        LOG_VAR(memoryProperties.memoryHeaps[i].size);
    }
    LOG_VAR(memoryProperties.memoryTypeCount);
#endif
			for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
			{
#if 0
        LOG_VAR(i);
        LOG_VAR(memoryProperties.memoryTypes[i].heapIndex);
        LOG_VAR(memoryProperties.memoryTypes[i].propertyFlags);
#endif
				if ((typeIndexFilter & (1 << i)) && (properties & memoryProperties.memoryTypes[i].propertyFlags) == properties)
					return i;
			}
			THROW("failed to find suitable memory type");
		}

		VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex)
		{
			VkMemoryAllocateInfo allocateInfo{
				VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				nullptr,
				memsize,
				memoryTypeIndex};

			VkDeviceMemory allocatedMemory;
			if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, &allocatedMemory) != VK_SUCCESS)
				THROW("failed to allocate vertex memory");
			return allocatedMemory;
		}
		void createCommandBuffers(
			VkDevice logicalDevice,
			VkCommandPool commandPool,
			uint32_t count,
			VkCommandBufferLevel level,
			std::vector<VkCommandBuffer> &commandBuffers)
		{
			commandBuffers.resize(count);
			VkCommandBufferAllocateInfo allocateInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				nullptr,
				commandPool,
				level,
				count};
			if (vkAllocateCommandBuffers(logicalDevice, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
				THROW("failed to create command buffer");
		}

		VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, uint32_t maxSetCount, const std::vector<VkDescriptorPoolSize> &descriptorPoolSizes)
		{
			VkDescriptorPoolCreateInfo createInfo{
				VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
				nullptr,
				0,
				maxSetCount,
				static_cast<uint32_t>(descriptorPoolSizes.size()),
				descriptorPoolSizes.data()};
			VkDescriptorPool ret;
			if (vkCreateDescriptorPool(logicalDevice, &createInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("failed to create DescriptorPool");
			return ret;
		}

		void createDescriptorSets(VkDevice logicalDevice, VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts, std::vector<VkDescriptorSet> &descriptorSets)
		{
			VkDescriptorSetAllocateInfo allocInfo{
				VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				nullptr,
				descriptorPool,
				static_cast<uint32_t>(descriptorSetLayouts.size()),
				descriptorSetLayouts.data()};
			if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
				THROW("failed to allocate descriptor set");
		}
		void createImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo, VkMemoryPropertyFlags properties, VkImage &outImage, VkDeviceMemory &outImageMemory)
		{
			if (vkCreateImage(logicalDevice, &imageCreateInfo, nullptr, &outImage) != VK_SUCCESS)
				THROW("failed to create image");

			VkMemoryRequirements imageMemoryRequireMents;
			vkGetImageMemoryRequirements(logicalDevice, outImage, &imageMemoryRequireMents);
			LOG_VAR(imageMemoryRequireMents.size);
			LOG_VAR(imageMemoryRequireMents.alignment);
			LOG_VAR(imageMemoryRequireMents.memoryTypeBits);

			auto memoryTypeIndex = findMemoryTypeIndex(physicalDevice, imageMemoryRequireMents.memoryTypeBits, properties);

			outImageMemory = allocateMemory(logicalDevice, imageMemoryRequireMents.size, memoryTypeIndex);

			vkBindImageMemory(logicalDevice, outImage, outImageMemory, 0);
		}

		VkCommandBuffer beginOneTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool)
		{
			std::vector<VkCommandBuffer> commandBuffers;
			createCommandBuffers(logicalDevice, commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffers);

			VkCommandBufferBeginInfo beginInfo{
				VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				nullptr,
				VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

			if (vkBeginCommandBuffer(commandBuffers[0], &beginInfo) != VK_SUCCESS)
				THROW("failed to begin command buffer");
			return commandBuffers[0];
		}

		void endOneTimeCommands(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer)
		{
			vkEndCommandBuffer(commandBuffer);
			VkSubmitInfo submitInfo{
				VK_STRUCTURE_TYPE_SUBMIT_INFO,
			};
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;
			if (vkQueueSubmit(queue, 1, &submitInfo, 0) != VK_SUCCESS)
				THROW("failed to submit one time commands");
			vkQueueWaitIdle(queue);
			vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
		}
		VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
		{
			for (auto format : candidates)
			{
				VkFormatProperties props;
				vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
				LOG_VAR(format);
				LOG_VAR(props.bufferFeatures);
				LOG_VAR(props.linearTilingFeatures);
				LOG_VAR(props.optimalTilingFeatures);
				if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
				{
					return format;
				}
			}
			THROW("failed to find supported format");
		}
		void generateMipmaps(VkCommandBuffer commandBuffer, VkImage image, int32_t width, int32_t height, uint32_t mipLevels, VkFilter filter)
		{
			VkImageMemoryBarrier barrier{
				VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				nullptr,
				0,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_QUEUE_FAMILY_IGNORED,
				VK_QUEUE_FAMILY_IGNORED,
				image,
				{VK_IMAGE_ASPECT_COLOR_BIT,
				 0,
				 1,
				 0,
				 1}};

			VkImageBlit blitRegion{
				{VK_IMAGE_ASPECT_COLOR_BIT,
				 0,
				 0,
				 1},
				{{}, {}},
				{VK_IMAGE_ASPECT_COLOR_BIT,
				 1,
				 0,
				 1},
				{{}, {}}};

			int32_t mipWidth = width, mipHeight = height;
			for (uint32_t i = 1; i < mipLevels; ++i)
			{
				barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				barrier.subresourceRange.baseMipLevel = i - 1;

				vkCmdPipelineBarrier(commandBuffer,
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 VK_PIPELINE_STAGE_TRANSFER_BIT,
									 0,
									 0, nullptr,
									 0, nullptr,
									 1, &barrier);

				blitRegion.srcSubresource.mipLevel = i - 1;
				blitRegion.srcOffsets[1] = {mipWidth, mipHeight, 1};

				mipWidth = (std::max)(mipWidth >> 1, 1);
				mipHeight = (std::max)(mipHeight >> 1, 1);
				blitRegion.dstSubresource.mipLevel = i;
				blitRegion.dstOffsets[1] = {mipWidth, mipHeight, 1};
				vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, filter);
			}
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.subresourceRange.baseMipLevel = mipLevels - 1;

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = mipLevels;

			vkCmdPipelineBarrier(commandBuffer,
								 VK_PIPELINE_STAGE_TRANSFER_BIT,
								 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
								 0,
								 0, nullptr,
								 0, nullptr,
								 1, &barrier);
		}

		VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(physicalDevice, &properties);

			VkSampleCountFlags maxSampleCount = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
			LOG_VAR(maxSampleCount);
			if (maxSampleCount & VK_SAMPLE_COUNT_64_BIT)
				return VK_SAMPLE_COUNT_64_BIT;
			if (maxSampleCount & VK_SAMPLE_COUNT_32_BIT)
				return VK_SAMPLE_COUNT_32_BIT;
			if (maxSampleCount & VK_SAMPLE_COUNT_16_BIT)
				return VK_SAMPLE_COUNT_16_BIT;
			if (maxSampleCount & VK_SAMPLE_COUNT_8_BIT)
				return VK_SAMPLE_COUNT_8_BIT;
			if (maxSampleCount & VK_SAMPLE_COUNT_4_BIT)
				return VK_SAMPLE_COUNT_4_BIT;
			if (maxSampleCount & VK_SAMPLE_COUNT_2_BIT)
				return VK_SAMPLE_COUNT_2_BIT;
			return VK_SAMPLE_COUNT_1_BIT;
		}

		void queryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties)
		{
			uint32_t count;
			vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
			extensionProperties.resize(count);
			vkEnumerateInstanceExtensionProperties(layerName, &count, extensionProperties.data());
		}
		void queryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties)
		{
			uint32_t count;
			vkEnumerateInstanceLayerProperties(&count, nullptr);
			layerProperties.resize(count);
			vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
		}
		void queryPhysicalDevices(VkInstance instance, std::vector<PhysicalDevice> &physicalDevices)
		{
			uint32_t count{};
			vkEnumeratePhysicalDevices(instance, &count, nullptr);
			std::vector<VkPhysicalDevice> devices(count);
			vkEnumeratePhysicalDevices(instance, &count, reinterpret_cast<VkPhysicalDevice *>(physicalDevices.data()));
		}
		void queryDeviceExtensionProperties(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties> &extensionProperies)
		{
			uint32_t extensionPropertyCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
			extensionProperies.resize(extensionPropertyCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperies.data());
		}
	}

	constexpr VkFormat vkFormatArray[]{
		VK_FORMAT_UNDEFINED,

		VK_FORMAT_R8_UNORM,
		VK_FORMAT_R8_SRGB,

		VK_FORMAT_R8G8_UNORM,
		VK_FORMAT_R8G8_SRGB,

		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_SRGB,
		VK_FORMAT_B8G8R8_UNORM,
		VK_FORMAT_B8G8R8_SRGB,

		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_UNORM,
		VK_FORMAT_B8G8R8A8_SRGB,

		VK_FORMAT_D16_UNORM,
		VK_FORMAT_X8_D24_UNORM_PACK32,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D24_UNORM_S8_UINT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_S8_UINT,
	};
	constexpr VkColorSpaceKHR vkColorSpaceArray[]{
		VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		VK_COLOR_SPACE_DISPLAY_P3_NONLINEAR_EXT,
		VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT,
		VK_COLOR_SPACE_DISPLAY_P3_LINEAR_EXT,
		VK_COLOR_SPACE_DCI_P3_NONLINEAR_EXT,
		VK_COLOR_SPACE_BT709_LINEAR_EXT,
		VK_COLOR_SPACE_BT709_NONLINEAR_EXT,
		VK_COLOR_SPACE_BT2020_LINEAR_EXT,
		VK_COLOR_SPACE_HDR10_ST2084_EXT,
		VK_COLOR_SPACE_DOLBYVISION_EXT,
		VK_COLOR_SPACE_HDR10_HLG_EXT,
		VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT,
		VK_COLOR_SPACE_ADOBERGB_NONLINEAR_EXT,
		VK_COLOR_SPACE_PASS_THROUGH_EXT,
		VK_COLOR_SPACE_EXTENDED_SRGB_NONLINEAR_EXT,
		VK_COLOR_SPACE_DISPLAY_NATIVE_AMD,
	};

	constexpr VkPresentModeKHR vkPresentModeArray[]{
		VK_PRESENT_MODE_IMMEDIATE_KHR,
		VK_PRESENT_MODE_FIFO_KHR,
	};
	constexpr VkBufferUsageFlagBits vkBufferUsageFlagBitArray[]{
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
	};
	constexpr VkMemoryPropertyFlagBits vkMemoryPropertyFlagBitArray[]{
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
		VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
		VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
	};
	constexpr VkQueueFlagBits vkQueueFlagBitArray[]{
		VK_QUEUE_GRAPHICS_BIT,
		VK_QUEUE_COMPUTE_BIT,
		VK_QUEUE_TRANSFER_BIT,
		VK_QUEUE_SPARSE_BINDING_BIT,
		// Provided by VK_VERSION_1_1
		VK_QUEUE_PROTECTED_BIT,
	};
	constexpr VkImageCreateFlagBits vkImageCreateFlagBitArray[]{
		VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT};

	constexpr VkImageType vkImageTypeArray[]{
		VK_IMAGE_TYPE_1D,
		VK_IMAGE_TYPE_2D,
		VK_IMAGE_TYPE_3D,
	};
	constexpr VkImageViewType vkImageViewTypeArray[]{
		VK_IMAGE_VIEW_TYPE_1D,
		VK_IMAGE_VIEW_TYPE_2D,
		VK_IMAGE_VIEW_TYPE_3D,
		VK_IMAGE_VIEW_TYPE_CUBE,
		VK_IMAGE_VIEW_TYPE_1D_ARRAY,
		VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
	};
	constexpr VkImageTiling vkImageTilingArray[]{
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_TILING_LINEAR};
	constexpr VkImageUsageFlagBits vkImageUsageFlagBitArray[]{
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_IMAGE_USAGE_STORAGE_BIT,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT};
	constexpr VkImageLayout vkImageLayoutArray[]{
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PREINITIALIZED,
		VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
		VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
		VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV,
		VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
		VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL_KHR,
		VK_IMAGE_LAYOUT_MAX_ENUM};
	constexpr VkCommandPoolCreateFlagBits vkCommandPoolCreateFlagBitArray[]{
		VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
	};
	constexpr VkFilter vkFilterArray[]{
		VK_FILTER_NEAREST,
		VK_FILTER_LINEAR,
		// Provided by VK_IMG_filter_cubic
		VK_FILTER_CUBIC_IMG,
		// Provided by VK_EXT_filter_cubic
		VK_FILTER_CUBIC_EXT,
	};
	constexpr VkDescriptorType vkDescriptorTypeArray[]{
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC,
		VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT};

	constexpr VkShaderStageFlagBits vkShaderStageFlagBitArray[]{
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT,
		VK_SHADER_STAGE_ALL_GRAPHICS,
		VK_SHADER_STAGE_ALL,
		VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
		VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		VK_SHADER_STAGE_MISS_BIT_KHR,
		VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
		VK_SHADER_STAGE_CALLABLE_BIT_KHR,
		VK_SHADER_STAGE_TASK_BIT_NV,
		VK_SHADER_STAGE_MESH_BIT_NV,
	};

	constexpr VkSamplerMipmapMode vkSamplerMipmapModeArray[]{
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_LINEAR};

	constexpr VkSamplerAddressMode vkSamplerAddressModeArray[]{
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		//// Provided by VK_VERSION_1_2, VK_KHR_sampler_mirror_clamp_to_edge
		//VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE ,
		//VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR,
	};
	constexpr VkCompareOp vkCompareOpArray[]{
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_EQUAL,
		VK_COMPARE_OP_LESS_OR_EQUAL,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_NOT_EQUAL,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
		VK_COMPARE_OP_ALWAYS};

	constexpr VkAttachmentLoadOp vkAttachmentLoadOpArray[]{
		VK_ATTACHMENT_LOAD_OP_LOAD,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		VK_ATTACHMENT_LOAD_OP_MAX_ENUM};
	constexpr VkAttachmentStoreOp vkAttachmentStoreOpArray[]{
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
		VK_ATTACHMENT_STORE_OP_NONE_QCOM,
		VK_ATTACHMENT_STORE_OP_MAX_ENUM};
	constexpr VkComponentSwizzle vkComponentSwizzleArray[]{
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_ZERO,
		VK_COMPONENT_SWIZZLE_ONE,
		VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A,
		VK_COMPONENT_SWIZZLE_MAX_ENUM};
	constexpr VkPipelineBindPoint vkPipelineBindPointArray[]{
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,
		VK_PIPELINE_BIND_POINT_MAX_ENUM};
	constexpr VkCommandBufferUsageFlagBits vkCommandBufferUsageFlagBitsArray[]{
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM};
	constexpr VkSubpassContents vkSubpassContentsArray[]{
		VK_SUBPASS_CONTENTS_INLINE,
		VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS,
	};
	constexpr VkIndexType vkIndexTypeArray[]{
		VK_INDEX_TYPE_NONE_KHR,	 // Provided by VK_KHR_ray_tracing
		VK_INDEX_TYPE_UINT8_EXT, // Provided by VK_EXT_index_type_uint8
		VK_INDEX_TYPE_UINT16,
		VK_INDEX_TYPE_UINT32,
	};
	constexpr VkCommandBufferResetFlagBits vkCommandBufferResetFlagBitsArray[]{
		VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT,
	};
	constexpr VkPrimitiveTopology vkPrimitiveTopologyArray[]{
		VK_PRIMITIVE_TOPOLOGY_POINT_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY,
		VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
	};
	constexpr VkPolygonMode vkPolygonModeArray[]{
		VK_POLYGON_MODE_FILL,
		VK_POLYGON_MODE_LINE,
		VK_POLYGON_MODE_POINT,
		VK_POLYGON_MODE_FILL_RECTANGLE_NV,
	};
	constexpr VkCullModeFlagBits vkCullModeFlagBitsArray[]{
		VK_CULL_MODE_NONE,
		VK_CULL_MODE_FRONT_BIT,
		VK_CULL_MODE_BACK_BIT,
		VK_CULL_MODE_FRONT_AND_BACK,
	};
	constexpr VkFrontFace vkFrontFaceArray[]{
		VK_FRONT_FACE_COUNTER_CLOCKWISE,
		VK_FRONT_FACE_CLOCKWISE,
	};
	constexpr VkStencilOp vkStencilOpArray[]{
		VK_STENCIL_OP_KEEP,
		VK_STENCIL_OP_ZERO,
		VK_STENCIL_OP_REPLACE,
		VK_STENCIL_OP_INCREMENT_AND_CLAMP,
		VK_STENCIL_OP_DECREMENT_AND_CLAMP,
		VK_STENCIL_OP_INVERT,
		VK_STENCIL_OP_INCREMENT_AND_WRAP,
		VK_STENCIL_OP_DECREMENT_AND_WRAP,
	};
	constexpr VkLogicOp vkLogicOpArray[]{
		VK_LOGIC_OP_CLEAR,
		VK_LOGIC_OP_AND,
		VK_LOGIC_OP_AND_REVERSE,
		VK_LOGIC_OP_COPY,
		VK_LOGIC_OP_AND_INVERTED,
		VK_LOGIC_OP_NO_OP,
		VK_LOGIC_OP_XOR,
		VK_LOGIC_OP_OR,
		VK_LOGIC_OP_NOR,
		VK_LOGIC_OP_EQUIVALENT,
		VK_LOGIC_OP_INVERT,
		VK_LOGIC_OP_OR_REVERSE,
		VK_LOGIC_OP_COPY_INVERTED,
		VK_LOGIC_OP_OR_INVERTED,
		VK_LOGIC_OP_NAND,
		VK_LOGIC_OP_SET,
	};
	constexpr VkBlendFactor vkBlendFactorArray[]{
		VK_BLEND_FACTOR_ZERO,
		VK_BLEND_FACTOR_ONE,
		VK_BLEND_FACTOR_SRC_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
		VK_BLEND_FACTOR_DST_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
		VK_BLEND_FACTOR_SRC_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		VK_BLEND_FACTOR_DST_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
		VK_BLEND_FACTOR_CONSTANT_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
		VK_BLEND_FACTOR_CONSTANT_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
		VK_BLEND_FACTOR_SRC_ALPHA_SATURATE,
		VK_BLEND_FACTOR_SRC1_COLOR,
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
		VK_BLEND_FACTOR_SRC1_ALPHA,
		VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,
		VK_BLEND_FACTOR_MAX_ENUM};
	constexpr VkBlendOp vkBlendOpArray[]{
		VK_BLEND_OP_ADD,
		VK_BLEND_OP_SUBTRACT,
		VK_BLEND_OP_REVERSE_SUBTRACT,
		VK_BLEND_OP_MIN,
		VK_BLEND_OP_MAX,
		VK_BLEND_OP_ZERO_EXT,
		VK_BLEND_OP_SRC_EXT,
		VK_BLEND_OP_DST_EXT,
		VK_BLEND_OP_SRC_OVER_EXT,
		VK_BLEND_OP_DST_OVER_EXT,
		VK_BLEND_OP_SRC_IN_EXT,
		VK_BLEND_OP_DST_IN_EXT,
		VK_BLEND_OP_SRC_OUT_EXT,
		VK_BLEND_OP_DST_OUT_EXT,
		VK_BLEND_OP_SRC_ATOP_EXT,
		VK_BLEND_OP_DST_ATOP_EXT,
		VK_BLEND_OP_XOR_EXT,
		VK_BLEND_OP_MULTIPLY_EXT,
		VK_BLEND_OP_SCREEN_EXT,
		VK_BLEND_OP_OVERLAY_EXT,
		VK_BLEND_OP_DARKEN_EXT,
		VK_BLEND_OP_LIGHTEN_EXT,
		VK_BLEND_OP_COLORDODGE_EXT,
		VK_BLEND_OP_COLORBURN_EXT,
		VK_BLEND_OP_HARDLIGHT_EXT,
		VK_BLEND_OP_SOFTLIGHT_EXT,
		VK_BLEND_OP_DIFFERENCE_EXT,
		VK_BLEND_OP_EXCLUSION_EXT,
		VK_BLEND_OP_INVERT_EXT,
		VK_BLEND_OP_INVERT_RGB_EXT,
		VK_BLEND_OP_LINEARDODGE_EXT,
		VK_BLEND_OP_LINEARBURN_EXT,
		VK_BLEND_OP_VIVIDLIGHT_EXT,
		VK_BLEND_OP_LINEARLIGHT_EXT,
		VK_BLEND_OP_PINLIGHT_EXT,
		VK_BLEND_OP_HARDMIX_EXT,
		VK_BLEND_OP_HSL_HUE_EXT,
		VK_BLEND_OP_HSL_SATURATION_EXT,
		VK_BLEND_OP_HSL_COLOR_EXT,
		VK_BLEND_OP_HSL_LUMINOSITY_EXT,
		VK_BLEND_OP_PLUS_EXT,
		VK_BLEND_OP_PLUS_CLAMPED_EXT,
		VK_BLEND_OP_PLUS_CLAMPED_ALPHA_EXT,
		VK_BLEND_OP_PLUS_DARKER_EXT,
		VK_BLEND_OP_MINUS_EXT,
		VK_BLEND_OP_MINUS_CLAMPED_EXT,
		VK_BLEND_OP_CONTRAST_EXT,
		VK_BLEND_OP_INVERT_OVG_EXT,
		VK_BLEND_OP_RED_EXT,
		VK_BLEND_OP_GREEN_EXT,
		VK_BLEND_OP_BLUE_EXT,
	};
	constexpr VkColorComponentFlagBits vkColorComponentFlagBitsArray[]{
		VK_COLOR_COMPONENT_R_BIT,
		VK_COLOR_COMPONENT_G_BIT,
		VK_COLOR_COMPONENT_B_BIT,
		VK_COLOR_COMPONENT_A_BIT,
	};
	constexpr VkDynamicState vkDynamicStateArray[]{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
		VK_DYNAMIC_STATE_BLEND_CONSTANTS,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV,
		VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT,
		VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT,
		VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV,
		VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV,
		VK_DYNAMIC_STATE_EXCLUSIVE_SCISSOR_NV,
		VK_DYNAMIC_STATE_LINE_STIPPLE_EXT,
		VK_DYNAMIC_STATE_CULL_MODE_EXT,
		VK_DYNAMIC_STATE_FRONT_FACE_EXT,
		VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT,
		VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT,
		VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT,
		VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT,
		VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT,
		VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT,
		VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT,
		VK_DYNAMIC_STATE_STENCIL_OP_EXT,
	};
	constexpr VkResult vkResultArray[]{
		VK_SUCCESS,
		VK_NOT_READY,
		VK_TIMEOUT,
		VK_EVENT_SET,
		VK_EVENT_RESET,
		VK_INCOMPLETE,
		VK_ERROR_OUT_OF_HOST_MEMORY,
		VK_ERROR_OUT_OF_DEVICE_MEMORY,
		VK_ERROR_INITIALIZATION_FAILED,
		VK_ERROR_DEVICE_LOST,
		VK_ERROR_MEMORY_MAP_FAILED,
		VK_ERROR_LAYER_NOT_PRESENT,
		VK_ERROR_EXTENSION_NOT_PRESENT,
		VK_ERROR_FEATURE_NOT_PRESENT,
		VK_ERROR_INCOMPATIBLE_DRIVER,
		VK_ERROR_TOO_MANY_OBJECTS,
		VK_ERROR_FORMAT_NOT_SUPPORTED,
		VK_ERROR_FRAGMENTED_POOL,
		VK_ERROR_UNKNOWN,
		VK_ERROR_OUT_OF_POOL_MEMORY,
		VK_ERROR_INVALID_EXTERNAL_HANDLE,
		VK_ERROR_FRAGMENTATION,
		VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS,
		VK_ERROR_SURFACE_LOST_KHR,
		VK_ERROR_NATIVE_WINDOW_IN_USE_KHR,
		VK_SUBOPTIMAL_KHR,
		VK_ERROR_OUT_OF_DATE_KHR,
		VK_ERROR_INCOMPATIBLE_DISPLAY_KHR,
		VK_ERROR_VALIDATION_FAILED_EXT,
		VK_ERROR_INVALID_SHADER_NV,
		VK_ERROR_INCOMPATIBLE_VERSION_KHR,
		VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
		VK_ERROR_NOT_PERMITTED_EXT,
		VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,
		VK_THREAD_IDLE_KHR,
		VK_THREAD_DONE_KHR,
		VK_OPERATION_DEFERRED_KHR,
		VK_OPERATION_NOT_DEFERRED_KHR,
		VK_PIPELINE_COMPILE_REQUIRED_EXT,
	};
	VkFenceCreateFlagBits VkFenceCreateFlagBitsArray[]{
		VK_FENCE_CREATE_SIGNALED_BIT,
	};
	VkSemaphoreType vkSemaphoreTypeArray[]{
		VK_SEMAPHORE_TYPE_BINARY,
		VK_SEMAPHORE_TYPE_TIMELINE,
	};
	VkSemaphoreType Map(SemaphoreType type)
	{
		return vkSemaphoreTypeArray[static_cast<size_t>(type)];
	}
	VkFenceCreateFlags Map(FenceCreateFlagBits flags)
	{
		VkFenceCreateFlags ret{};
		int a = static_cast<int>(flags);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= VkFenceCreateFlagBitsArray[i];
		}
		return ret;
	};
	VkDynamicState Map(DynamicState state)
	{
		return vkDynamicStateArray[static_cast<size_t>(state)];
	}
	VkColorComponentFlags Map(ColorComponentFlagBits flag)
	{
		return static_cast<VkColorComponentFlags>(flag);
	}
	VkBlendOp Map(BlendOp op)
	{
		return vkBlendOpArray[static_cast<size_t>(op)];
	}
	VkBlendFactor Map(BlendFactor factor)
	{
		return vkBlendFactorArray[static_cast<size_t>(factor)];
	}
	VkLogicOp Map(LogicOp op)
	{
		return vkLogicOpArray[static_cast<size_t>(op)];
	}
	VkStencilOp Map(StencilOp op)
	{
		return vkStencilOpArray[static_cast<size_t>(op)];
	}
	VkFrontFace Map(FrontFace face)
	{
		return vkFrontFaceArray[static_cast<size_t>(face)];
	}
	VkCullModeFlags Map(CullMode cullmode)
	{
		return static_cast<VkCullModeFlags>(vkCullModeFlagBitsArray[static_cast<size_t>(cullmode)]);
	}
	VkPolygonMode Map(PolygonMode mode)
	{
		return vkPolygonModeArray[static_cast<size_t>(mode)];
	}
	VkPrimitiveTopology Map(PrimitiveTopology topology)
	{
		return vkPrimitiveTopologyArray[static_cast<size_t>(topology)];
	}

	VkCommandBufferResetFlags Map(CommandBufferResetFlatBits flag)
	{
		VkCommandBufferResetFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkCommandBufferResetFlagBitsArray[i];
		}
		return ret;
	}
	VkIndexType Map(IndexType type)
	{
		return vkIndexTypeArray[static_cast<size_t>(type)];
	}
	VkSubpassContents Map(SubpassContents contents)
	{
		return vkSubpassContentsArray[static_cast<size_t>(contents)];
	}

	VkPipelineBindPoint Map(PipelineBindPoint bindPoint)
	{
		return vkPipelineBindPointArray[static_cast<size_t>(bindPoint)];
	}
	VkComponentSwizzle Map(ComponentSwizzle swizzle)
	{
		return vkComponentSwizzleArray[static_cast<size_t>(swizzle)];
	}
	VkAttachmentLoadOp Map(AttachmentLoadOp op)
	{
		return vkAttachmentLoadOpArray[static_cast<size_t>(op)];
	}
	VkAttachmentStoreOp Map(AttachmentStoreOp op)
	{
		return vkAttachmentStoreOpArray[static_cast<size_t>(op)];
	}
	VkCompareOp Map(CompareOp op)
	{
		return vkCompareOpArray[static_cast<size_t>(op)];
	}
	VkSamplerAddressMode Map(SamplerWrapMode mode)
	{
		return vkSamplerAddressModeArray[static_cast<size_t>(mode)];
	}
	VkSamplerMipmapMode Map(SamplerMipmapMode mode)
	{
		return vkSamplerMipmapModeArray[static_cast<size_t>(mode)];
	}
	VkShaderStageFlagBits Map(ShaderStageFlagBits flag)
	{
		return static_cast<VkShaderStageFlagBits>(flag);
	}
	VkDescriptorType Map(DescriptorType type)
	{
		return vkDescriptorTypeArray[static_cast<size_t>(type)];
	}

	VkFilter Map(Filter filter)
	{
		return vkFilterArray[static_cast<size_t>(filter)];
	}
	VkCommandPoolCreateFlags Map(CommandPoolCreateFlagBits flag)
	{
		VkCommandPoolCreateFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkCommandPoolCreateFlagBitArray[i];
		}
		return ret;
	}

	VkImageCreateFlags Map(ImageCreateFlagBits flag)
	{
		VkImageCreateFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkImageCreateFlagBitArray[i];
		}
		return ret;
	}
	VkImageType Map(ImageType type)
	{
		return vkImageTypeArray[static_cast<size_t>(type)];
	}
	VkImageViewType Map(ImageViewType type)
	{
		return vkImageViewTypeArray[static_cast<size_t>(type)];
	}
	VkImageTiling Map(ImageTiling tiling)
	{
		return vkImageTilingArray[static_cast<size_t>(tiling)];
	}
	VkImageUsageFlags Map(ImageUsageFlagBits flag)
	{
		VkImageUsageFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkImageUsageFlagBitArray[i];
		}
		return ret;
	}
	VkImageLayout Map(ImageLayout layout)
	{
		return vkImageLayoutArray[static_cast<size_t>(layout)];
	}

	VkBufferUsageFlags Map(BufferUsageFlagBits flag)
	{
		VkBufferUsageFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkBufferUsageFlagBitArray[i];
		}
		return ret;
	}

	VkMemoryPropertyFlags Map(MemoryPropertyFlagBits flag)
	{
		VkMemoryPropertyFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkMemoryPropertyFlagBitArray[i];
		}
		return ret;
	}

	VkFormat Map(ShitFormat format)
	{
		return vkFormatArray[static_cast<size_t>(format)];
	}
	ShitFormat Map(VkFormat format)
	{
		static std::unordered_map<VkFormat, ShitFormat> tempMap;
		if (tempMap.find(format) == tempMap.end())
		{
			for (int i = 0, len = static_cast<int>(ShitFormat::Num); i < len; ++i)
			{
				auto a = Map(static_cast<ShitFormat>(i));
				if (a == format)
					return tempMap[format] = static_cast<ShitFormat>(i);
			}
			THROW("failed to find corresponding ShitFormat");
		}
		else
			return tempMap[format];
	};

	VkColorSpaceKHR Map(ColorSpace colorSpace)
	{
		return vkColorSpaceArray[static_cast<size_t>(colorSpace)];
	}
	ColorSpace Map(VkColorSpaceKHR colorSpace)
	{
		static std::unordered_map<VkColorSpaceKHR, ColorSpace> tempMap;
		if (tempMap.find(colorSpace) == tempMap.end())
		{
			for (int i = 0, len = static_cast<int>(ShitFormat::Num); i < len; ++i)
			{
				auto a = Map(static_cast<ColorSpace>(i));
				if (a == colorSpace)
					return tempMap[a] = static_cast<ColorSpace>(i);
			}
			THROW("failed to find corresponding ShitFormat");
		}
		else
			return tempMap[colorSpace];
	}

	VkPresentModeKHR Map(PresentMode mode)
	{
		return vkPresentModeArray[static_cast<size_t>(mode)];
	}
	VkCommandBufferLevel Map(CommandBufferLevel level)
	{
		if (level == CommandBufferLevel::SECONDARY)
			return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
		else
			return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	}
	VkQueueFlags Map(QueueFlagBits flag)
	{
		int a = static_cast<int>(flag);
		VkQueueFlags ret{};
		for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
		{
			if (a & 1)
				ret |= vkQueueFlagBitArray[i];
		}
		return ret;
	}

	VkImageAspectFlags GetImageAspectFromFormat(ShitFormat format)
	{
		switch (format)
		{
		case ShitFormat::D16_UNORM:
		case ShitFormat::D24_UNORM:
		case ShitFormat::D32_SFLOAT:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		case ShitFormat::D24_UNORM_S8_UINT:
		case ShitFormat::D32_SFLOAT_S8_UINT:
			return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		case ShitFormat::S8_UINT:
			return VK_IMAGE_ASPECT_STENCIL_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		}
	}
	VkFormat GetFormat(DataType dataType, uint32_t components, bool normalized)
	{
		if (normalized)
		{
			switch (components)
			{
			case 1:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8_SNORM;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8_UNORM;
				case DataType::SHORT:
					return VK_FORMAT_R16_SNORM;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16_UNORM;
				default:
					break;
				}
				break;
			case 2:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8_SNORM;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8_UNORM;
				case DataType::SHORT:
					return VK_FORMAT_R16G16_SNORM;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16_UNORM;
				default:
					break;
				}
				break;
			case 3:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8B8_SNORM;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8B8_UNORM;
				case DataType::SHORT:
					return VK_FORMAT_R16G16B16_SNORM;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16B16_UNORM;
				default:
					break;
				}
				break;
			case 4:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8B8A8_SNORM;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8B8A8_UNORM;
				case DataType::SHORT:
					return VK_FORMAT_R16G16B16A16_SNORM;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16B16A16_UNORM;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		else
		{
			switch (components)
			{
			case 1:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8_SINT;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8_UINT;
				case DataType::SHORT:
					return VK_FORMAT_R16_SINT;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16_UINT;
				case DataType::INT:
					return VK_FORMAT_R32_SINT;
				case DataType::UNSIGNED_INT:
					return VK_FORMAT_R32_UINT;
				case DataType::FLOAT_HALF:
					return VK_FORMAT_R16_SFLOAT;
				case DataType::FLOAT:
					return VK_FORMAT_R32_SFLOAT;
				case DataType::DOUBLE:
					return VK_FORMAT_R64_SFLOAT;
				default:
					break;
				}
				break;
			case 2:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8_SINT;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8_UINT;
				case DataType::SHORT:
					return VK_FORMAT_R16G16_SINT;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16_UINT;
				case DataType::INT:
					return VK_FORMAT_R32G32_SINT;
				case DataType::UNSIGNED_INT:
					return VK_FORMAT_R32G32_UINT;
				case DataType::FLOAT_HALF:
					return VK_FORMAT_R16G16_SFLOAT;
				case DataType::FLOAT:
					return VK_FORMAT_R32G32_SFLOAT;
				case DataType::DOUBLE:
					return VK_FORMAT_R64G64_SFLOAT;
				default:
					break;
				}
				break;
			case 3:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8B8_SINT;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8B8_UINT;
				case DataType::SHORT:
					return VK_FORMAT_R16G16B16_SINT;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16B16_UINT;
				case DataType::INT:
					return VK_FORMAT_R32G32B32_SINT;
				case DataType::UNSIGNED_INT:
					return VK_FORMAT_R32G32B32_UINT;
				case DataType::FLOAT_HALF:
					return VK_FORMAT_R16G16B16_SFLOAT;
				case DataType::FLOAT:
					return VK_FORMAT_R32G32B32_SFLOAT;
				case DataType::DOUBLE:
					return VK_FORMAT_R64G64B64_SFLOAT;
				default:
					break;
				}
				break;
			case 4:
				switch (dataType)
				{
				case DataType::BYTE:
					return VK_FORMAT_R8G8B8A8_SINT;
				case DataType::UNSIGNED_BYTE:
					return VK_FORMAT_R8G8B8A8_UINT;
				case DataType::SHORT:
					return VK_FORMAT_R16G16B16A16_SINT;
				case DataType::UNSIGNED_SHORT:
					return VK_FORMAT_R16G16B16A16_UINT;
				case DataType::INT:
					return VK_FORMAT_R32G32B32A32_SINT;
				case DataType::UNSIGNED_INT:
					return VK_FORMAT_R32G32B32A32_UINT;
				case DataType::FLOAT_HALF:
					return VK_FORMAT_R16G16B16A16_SFLOAT;
				case DataType::FLOAT:
					return VK_FORMAT_R32G32B32A32_SFLOAT;
				case DataType::DOUBLE:
					return VK_FORMAT_R64G64B64A64_SFLOAT;
				default:
					break;
				}
				break;
			default:
				break;
			}
		}
		THROW("failed to faind appropriate format");
	}

} // namespace Shit
