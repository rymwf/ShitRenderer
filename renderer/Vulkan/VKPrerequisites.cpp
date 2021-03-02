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
			LOG_VAR(deviceProperties.pipelineCacheUUID);
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

		std::optional<uint32_t> findQueueFamilyIndexByFlag(std::vector<VkQueueFamilyProperties> &queueFamilyProperties, VkQueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices)
		{
			for (uint32_t i = 0, l = static_cast<uint32_t>(queueFamilyProperties.size()); i < l; ++i)
			{
				if (skipIndices.find(i) != skipIndices.end())
					continue;
				if (queueFamilyProperties[i].queueFlags & flag)
				{
					return std::optional<uint32_t>(i);
				}
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
		VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice, const std::vector<uint32_t> &queueFamilyIndices)
		{
			//show physical device extensions
			uint32_t extensionPropertyCount;
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
			std::vector<VkExtensionProperties> properties(extensionPropertyCount);
			vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, properties.data());

			std::vector<const char *> extensionNames;
			extensionNames.reserve(extensionPropertyCount);
			LOG_VAR(extensionPropertyCount);
			for (auto &extensionProperty : properties)
			{
				LOG_VAR(extensionProperty.extensionName);
				LOG_VAR(extensionProperty.specVersion);
				extensionNames.emplace_back(extensionProperty.extensionName);
			}

			//physical device  features
			VkPhysicalDeviceFeatures deviceFeatures;
			vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

			deviceFeatures.sampleRateShading = true;

			const float queuePriorities = 1.0;
			std::vector<VkDeviceQueueCreateInfo> queueInfos;
			for (auto i : queueFamilyIndices)
			{
				queueInfos.emplace_back(
					VkDeviceQueueCreateInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
											NULL,
											0,
											i, //queue family index
											1, //queue count
											&queuePriorities});
			}

			VkDeviceCreateInfo deviceInfo{
				VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				NULL,
				0,
				static_cast<uint32_t>(queueInfos.size()),
				queueInfos.data(),
				0, //deprecated
				0, //deprecated
				extensionPropertyCount,
				extensionNames.data(),
				&deviceFeatures};
			VkDevice ret;
			if (vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &ret) != VK_SUCCESS)
				THROW("create logical device failed");

			return ret;
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
				if (typeIndexFilter & (1 << i) && properties & memoryProperties.memoryTypes[i].propertyFlags)
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
		void createCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count, VkCommandBufferLevel level, std::vector<VkCommandBuffer> &commandBuffers)
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

	VkBufferUsageFlags Map(BufferUsageFlagBits flag)
	{
		VkBufferUsageFlags ret{};
		int a = static_cast<int>(flag);
		for (int i = 0; a > 0 && i < 32; ++i, flag >>= 1)
		{
			if (a & 1)
				ret |= vkBufferUsageFlagBitArray[i];
		}
		return ret;
	}

	VkFormat Map(ShitFormat format)
	{
		return vkFormatArray[static_cast<size_t>(format)];
	}

	VkColorSpaceKHR Map(ColorSpace colorSpace)
	{
		return vkColorSpaceArray[static_cast<size_t>(colorSpace)];
	}

	VkPresentModeKHR Map(PresentMode mode)
	{
		return vkPresentModeArray[static_cast<size_t>(mode)];
	}

} // namespace Shit
