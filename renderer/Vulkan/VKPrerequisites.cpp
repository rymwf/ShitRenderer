/**
 * @file VKPrerequisites.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKPrerequisites.hpp"
namespace Shit {
namespace VK {
int rateDeviceSuitability(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    ST_LOG_VAR(deviceProperties.apiVersion);
    ST_LOG_VAR(VK_VERSION_MAJOR(deviceProperties.apiVersion));
    ST_LOG_VAR(VK_VERSION_MINOR(deviceProperties.apiVersion));
    ST_LOG_VAR(VK_VERSION_PATCH(deviceProperties.apiVersion));
    ST_LOG_VAR(deviceProperties.driverVersion);
    ST_LOG_VAR(deviceProperties.vendorID);
    ST_LOG_VAR(deviceProperties.deviceID);
    ST_LOG_VAR(deviceProperties.deviceType);
    ST_LOG_VAR(deviceProperties.deviceName);
    //			ST_LOG_VAR(deviceProperties.pipelineCacheUUID);
    ST_LOG_VAR(deviceProperties.limits.maxVertexInputBindings);
    ST_LOG_VAR(deviceProperties.limits.maxVertexInputBindingStride);
    ST_LOG_VAR(deviceProperties.limits.maxVertexInputAttributes);
    ST_LOG_VAR(deviceProperties.limits.maxImageDimension2D);
    //       ST_LOG_VAR(deviceProperties.sparseProperties);

    ST_LOG_VAR(deviceFeatures.geometryShader);
    ST_LOG_VAR(deviceFeatures.samplerAnisotropy);

    if (!deviceFeatures.geometryShader) {
        std::fprintf(stderr, "Info: Discarding device '%s': geometry not supported\n", deviceProperties.deviceName);
        return -1;
    }

    // Only consider Vulkan 1.1 devices
    auto const major = VK_API_VERSION_MAJOR(deviceProperties.apiVersion);
    auto const minor = VK_API_VERSION_MINOR(deviceProperties.apiVersion);

    if (major < 1 || (major == 1 && minor < 2)) {
        std::fprintf(stderr, "Info: Discarding device '%s': insufficient vulkan version\n",
                     deviceProperties.deviceName);
        return -1;
    }

    // get extensions
    if (surface) {
        //get surface extensions
        std::vector<VkExtensionProperties> deviceExtensions;
        queryDeviceExtensionProperties(physicalDevice, deviceExtensions);

        auto it = std::find_if(deviceExtensions.begin(), deviceExtensions.end(),
                               [](auto &&e) { return strcmp(e.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0; });
        if (it == deviceExtensions.end()) {
            std::fprintf(stderr, "Info: Discarding device '%s': extension %s missing\n", deviceProperties.deviceName,
                         VK_KHR_SWAPCHAIN_EXTENSION_NAME);
            return -1;
        }

        uint32_t queueFamilyPropertyCount;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

        auto familyIndex = findQueueFamilyIndexPresent(physicalDevice, queueFamilyPropertyCount, surface);

        if (!familyIndex) {
            std::fprintf(stderr, "Info: Discarding device '%s': no present queue family\n",
                         deviceProperties.deviceName);
            return -1;
        }
    }

    // Application can't function without geometry shaders
    int score = 0;

    // discrete gpu is prefered
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;
    else if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 200;

    // Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    ST_LOG_VAR(deviceProperties.deviceName);
    return score;
}

std::optional<uint32_t> findQueueFamilyIndexByFlag(std::span<const VkQueueFamilyProperties> queueFamilyProperties,
                                                   VkQueueFlags flag) {
    std::vector<uint32_t> candidates;
    for (uint32_t i = 0, l = static_cast<uint32_t>(queueFamilyProperties.size()); i < l; ++i) {
        if (queueFamilyProperties[i].queueFlags == flag) return i;
        if ((queueFamilyProperties[i].queueFlags & flag) == flag) candidates.emplace_back(i);
    }
    if (!candidates.empty()) return candidates[0];
    return std::nullopt;
}
std::optional<uint32_t> findQueueFamilyIndexPresent(VkPhysicalDevice physicalDevice, uint32_t familyNum,
                                                    VkSurfaceKHR surface) {
    VkBool32 surfaceSupported = false;
    for (uint32_t i = 0; i < familyNum && !surfaceSupported; ++i) {
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &surfaceSupported);
        if (surfaceSupported) return std::optional<uint32_t>(i);
    }
    return std::nullopt;
}
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    ST_LOG_VAR(physicalDeviceCount);
    if (physicalDeviceCount == 0) {
        ST_THROW("failed to find GPUs with vulkan support");
    }
    // first element is score
    std::vector<VkPhysicalDevice> devices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, devices.data());

    std::vector<std::pair<int, VkPhysicalDevice>> scoreddevices;
    for (auto &device : devices) {
        scoreddevices.emplace_back(rateDeviceSuitability(device, surface), device);
    }
    std::sort(scoreddevices.begin(), scoreddevices.end());

    auto &temp = scoreddevices.back();
    if (temp.first <= 0) ST_THROW("failed to find a suitable GPU");

    return temp.second;
}
void queryPhysicalDeviceGroupInfo(VkInstance instance,
                                  std::vector<VkPhysicalDeviceGroupProperties> &physicalDeviceGroupProperties) {
    uint32_t physicalDeviceGroupCount;
    vkEnumeratePhysicalDeviceGroups(instance, &physicalDeviceGroupCount, nullptr);
    physicalDeviceGroupProperties.resize(physicalDeviceGroupCount);
    vkEnumeratePhysicalDeviceGroups(instance, &physicalDeviceGroupCount, physicalDeviceGroupProperties.data());
    ST_LOG_VAR(physicalDeviceGroupCount);
    for (auto physicalDeviceGroupProperty : physicalDeviceGroupProperties) {
        ST_LOG_VAR(physicalDeviceGroupProperty.physicalDeviceCount);
    }
}

void querySurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                         std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
    uint32_t surfaceFormatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, nullptr);
    surfaceFormats.resize(surfaceFormatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data());
#ifndef NDEBUG
    ST_LOG_VAR(surfaceFormatCount);
    for (auto &surfaceFormat : surfaceFormats) {
        ST_LOG("surfaceFormat.colorSpace:", surfaceFormat.colorSpace, "surfaceFormat.format:", surfaceFormat.format);
    }
#endif
}
void querySurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                              std::vector<VkPresentModeKHR> &presentModes) {
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);
    presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());
}
void queryDisplayProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPropertiesKHR> &displayProperties) {
    uint32_t count;
    vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &count, nullptr);
    displayProperties.resize(count);
    vkGetPhysicalDeviceDisplayPropertiesKHR(physicalDevice, &count, displayProperties.data());
#ifndef NDEBUG
    for (auto &displayProperty : displayProperties) {
        ST_LOG_VAR(displayProperty.display);
        ST_LOG_VAR(displayProperty.displayName);
        ST_LOG_VAR(displayProperty.persistentContent);
        ST_LOG_VAR(displayProperty.physicalDimensions.width);
        ST_LOG_VAR(displayProperty.physicalDimensions.height);
        ST_LOG_VAR(displayProperty.physicalResolution.width);
        ST_LOG_VAR(displayProperty.physicalResolution.height);
        ST_LOG_VAR(displayProperty.planeReorderPossible);
        ST_LOG_VAR(displayProperty.supportedTransforms);
    }
#endif
}
void queryDisplayPlaneProperties(VkPhysicalDevice physicalDevice,
                                 std::vector<VkDisplayPlanePropertiesKHR> &displayPlaneProperties) {
    uint32_t count;
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &count, nullptr);
    displayPlaneProperties.resize(count);
    vkGetPhysicalDeviceDisplayPlanePropertiesKHR(physicalDevice, &count, displayPlaneProperties.data());
#ifndef NDEBUG
    for (auto displayPlaneProperty : displayPlaneProperties) {
        ST_LOG_VAR(displayPlaneProperty.currentDisplay);
        ST_LOG_VAR(displayPlaneProperty.currentStackIndex);
    }
#endif
}
void queryDisplayModeProperties(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                std::vector<VkDisplayModePropertiesKHR> &displayModeProperties) {
    uint32_t count;
    vkGetDisplayModePropertiesKHR(physicalDevice, display, &count, nullptr);
    displayModeProperties.resize(count);
    vkGetDisplayModePropertiesKHR(physicalDevice, display, &count, displayModeProperties.data());
#ifndef NDEBUG
    for (auto displayModeProperty : displayModeProperties) {
        ST_LOG_VAR(displayModeProperty.displayMode);
        ST_LOG_VAR(displayModeProperty.parameters.refreshRate);
        ST_LOG_VAR(displayModeProperty.parameters.visibleRegion.width);
        ST_LOG_VAR(displayModeProperty.parameters.visibleRegion.height);
    }
#endif
}
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeIndexFilter,
                             VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
#if 0
    ST_LOG_VAR(memoryProperties.memoryHeapCount);
    for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; i++)
    {
        ST_LOG_VAR(i);
        ST_LOG_VAR(memoryProperties.memoryHeaps[i].flags);
        ST_LOG_VAR(memoryProperties.memoryHeaps[i].size);
    }
    ST_LOG_VAR(memoryProperties.memoryTypeCount);
#endif
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
#if 0
        ST_LOG_VAR(i);
        ST_LOG_VAR(memoryProperties.memoryTypes[i].heapIndex);
        ST_LOG_VAR(memoryProperties.memoryTypes[i].propertyFlags);
#endif
        if ((typeIndexFilter & (1 << i)) && (properties & memoryProperties.memoryTypes[i].propertyFlags) == properties)
            return i;
    }
    ST_THROW("failed to find suitable memory type");
}

VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex) {
    VkMemoryAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, memsize, memoryTypeIndex};

    VkDeviceMemory allocatedMemory;
    if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, &allocatedMemory) != VK_SUCCESS)
        ST_THROW("failed to allocate vertex memory");
    return allocatedMemory;
}
void createCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count, VkCommandBufferLevel level,
                          std::vector<VkCommandBuffer> &commandBuffers) {
    commandBuffers.resize(count);
    VkCommandBufferAllocateInfo allocateInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, nullptr, commandPool,
                                             level, count};
    if (vkAllocateCommandBuffers(logicalDevice, &allocateInfo, commandBuffers.data()) != VK_SUCCESS)
        ST_THROW("failed to create command buffer");
}
VkCommandBuffer beginOneTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool) {
    std::vector<VkCommandBuffer> commandBuffers;
    createCommandBuffers(logicalDevice, commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY, commandBuffers);

    VkCommandBufferBeginInfo beginInfo{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, nullptr,
                                       VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    if (vkBeginCommandBuffer(commandBuffers[0], &beginInfo) != VK_SUCCESS) ST_THROW("failed to begin command buffer");
    return commandBuffers[0];
}

void endOneTimeCommands(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool,
                        VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);
    VkSubmitInfo submitInfo{
        VK_STRUCTURE_TYPE_SUBMIT_INFO,
    };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    if (vkQueueSubmit(queue, 1, &submitInfo, 0) != VK_SUCCESS) ST_THROW("failed to submit one time commands");
    vkQueueWaitIdle(queue);
    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, std::span<const VkFormat> candidates,
                             VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (auto format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
        ST_LOG_VAR(format);
        ST_LOG_VAR(props.bufferFeatures);
        ST_LOG_VAR(props.linearTilingFeatures);
        ST_LOG_VAR(props.optimalTilingFeatures);
        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    ST_THROW("failed to find supported format");
}
void generateMipmaps(VkCommandBuffer commandBuffer, VkImage image, int32_t width, int32_t height, uint32_t mipLevels,
                     VkFilter filter) {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                                 nullptr,
                                 0,
                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 VK_QUEUE_FAMILY_IGNORED,
                                 image,
                                 {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

    VkImageBlit blitRegion{
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1}, {{}, {}}, {VK_IMAGE_ASPECT_COLOR_BIT, 1, 0, 1}, {{}, {}}};

    int32_t mipWidth = width, mipHeight = height;
    for (uint32_t i = 1; i < mipLevels; ++i) {
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.subresourceRange.baseMipLevel = i - 1;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        blitRegion.srcSubresource.mipLevel = i - 1;
        blitRegion.srcOffsets[1] = {mipWidth, mipHeight, 1};

        mipWidth = (std::max)(mipWidth >> 1, 1);
        mipHeight = (std::max)(mipHeight >> 1, 1);
        blitRegion.dstSubresource.mipLevel = i;
        blitRegion.dstOffsets[1] = {mipWidth, mipHeight, 1};
        vkCmdBlitImage(commandBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, filter);
    }
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) {
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSampleCountFlags maxSampleCount =
        properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    ST_LOG_VAR(maxSampleCount);
    if (maxSampleCount & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (maxSampleCount & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (maxSampleCount & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (maxSampleCount & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (maxSampleCount & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (maxSampleCount & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

void queryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties) {
    uint32_t count{};
    vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
    extensionProperties.resize(count);
    vkEnumerateInstanceExtensionProperties(layerName, &count, extensionProperties.data());
}
void queryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties) {
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    layerProperties.resize(count);
    vkEnumerateInstanceLayerProperties(&count, layerProperties.data());
}
void queryPhysicalDevices(VkInstance instance, std::vector<PhysicalDevice> &physicalDevices) {
    uint32_t count{};
    vkEnumeratePhysicalDevices(instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(instance, &count, reinterpret_cast<VkPhysicalDevice *>(physicalDevices.data()));
}
void queryDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                    std::vector<VkExtensionProperties> &extensionProperies) {
    uint32_t extensionPropertyCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
    extensionProperies.resize(extensionPropertyCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensionProperies.data());
}
}  // namespace VK

constexpr VkFormat vkFormatArray[]{
    VK_FORMAT_UNDEFINED,                                   // = 0,
    VK_FORMAT_R4G4_UNORM_PACK8,                            // = 1,
    VK_FORMAT_R4G4B4A4_UNORM_PACK16,                       // = 2,
    VK_FORMAT_B4G4R4A4_UNORM_PACK16,                       // = 3,
    VK_FORMAT_R5G6B5_UNORM_PACK16,                         // = 4,
    VK_FORMAT_B5G6R5_UNORM_PACK16,                         // = 5,
    VK_FORMAT_R5G5B5A1_UNORM_PACK16,                       // = 6,
    VK_FORMAT_B5G5R5A1_UNORM_PACK16,                       // = 7,
    VK_FORMAT_A1R5G5B5_UNORM_PACK16,                       // = 8,
    VK_FORMAT_R8_UNORM,                                    // = 9,
    VK_FORMAT_R8_SNORM,                                    // = 10,
    VK_FORMAT_R8_USCALED,                                  // = 11,
    VK_FORMAT_R8_SSCALED,                                  // = 12,
    VK_FORMAT_R8_UINT,                                     // = 13,
    VK_FORMAT_R8_SINT,                                     // = 14,
    VK_FORMAT_R8_SRGB,                                     // = 15,
    VK_FORMAT_R8G8_UNORM,                                  // = 16,
    VK_FORMAT_R8G8_SNORM,                                  // = 17,
    VK_FORMAT_R8G8_USCALED,                                // = 18,
    VK_FORMAT_R8G8_SSCALED,                                // = 19,
    VK_FORMAT_R8G8_UINT,                                   // = 20,
    VK_FORMAT_R8G8_SINT,                                   // = 21,
    VK_FORMAT_R8G8_SRGB,                                   // = 22,
    VK_FORMAT_R8G8B8_UNORM,                                // = 23,
    VK_FORMAT_R8G8B8_SNORM,                                // = 24,
    VK_FORMAT_R8G8B8_USCALED,                              // = 25,
    VK_FORMAT_R8G8B8_SSCALED,                              // = 26,
    VK_FORMAT_R8G8B8_UINT,                                 // = 27,
    VK_FORMAT_R8G8B8_SINT,                                 // = 28,
    VK_FORMAT_R8G8B8_SRGB,                                 // = 29,
    VK_FORMAT_B8G8R8_UNORM,                                // = 30,
    VK_FORMAT_B8G8R8_SNORM,                                // = 31,
    VK_FORMAT_B8G8R8_USCALED,                              // = 32,
    VK_FORMAT_B8G8R8_SSCALED,                              // = 33,
    VK_FORMAT_B8G8R8_UINT,                                 // = 34,
    VK_FORMAT_B8G8R8_SINT,                                 // = 35,
    VK_FORMAT_B8G8R8_SRGB,                                 // = 36,
    VK_FORMAT_R8G8B8A8_UNORM,                              // = 37,
    VK_FORMAT_R8G8B8A8_SNORM,                              // = 38,
    VK_FORMAT_R8G8B8A8_USCALED,                            // = 39,
    VK_FORMAT_R8G8B8A8_SSCALED,                            // = 40,
    VK_FORMAT_R8G8B8A8_UINT,                               // = 41,
    VK_FORMAT_R8G8B8A8_SINT,                               // = 42,
    VK_FORMAT_R8G8B8A8_SRGB,                               // = 43,
    VK_FORMAT_B8G8R8A8_UNORM,                              // = 44,
    VK_FORMAT_B8G8R8A8_SNORM,                              // = 45,
    VK_FORMAT_B8G8R8A8_USCALED,                            // = 46,
    VK_FORMAT_B8G8R8A8_SSCALED,                            // = 47,
    VK_FORMAT_B8G8R8A8_UINT,                               // = 48,
    VK_FORMAT_B8G8R8A8_SINT,                               // = 49,
    VK_FORMAT_B8G8R8A8_SRGB,                               // = 50,
    VK_FORMAT_A8B8G8R8_UNORM_PACK32,                       // = 51,
    VK_FORMAT_A8B8G8R8_SNORM_PACK32,                       // = 52,
    VK_FORMAT_A8B8G8R8_USCALED_PACK32,                     // = 53,
    VK_FORMAT_A8B8G8R8_SSCALED_PACK32,                     // = 54,
    VK_FORMAT_A8B8G8R8_UINT_PACK32,                        // = 55,
    VK_FORMAT_A8B8G8R8_SINT_PACK32,                        // = 56,
    VK_FORMAT_A8B8G8R8_SRGB_PACK32,                        // = 57,
    VK_FORMAT_A2R10G10B10_UNORM_PACK32,                    // = 58,
    VK_FORMAT_A2R10G10B10_SNORM_PACK32,                    // = 59,
    VK_FORMAT_A2R10G10B10_USCALED_PACK32,                  // = 60,
    VK_FORMAT_A2R10G10B10_SSCALED_PACK32,                  // = 61,
    VK_FORMAT_A2R10G10B10_UINT_PACK32,                     // = 62,
    VK_FORMAT_A2R10G10B10_SINT_PACK32,                     // = 63,
    VK_FORMAT_A2B10G10R10_UNORM_PACK32,                    // = 64,
    VK_FORMAT_A2B10G10R10_SNORM_PACK32,                    // = 65,
    VK_FORMAT_A2B10G10R10_USCALED_PACK32,                  // = 66,
    VK_FORMAT_A2B10G10R10_SSCALED_PACK32,                  // = 67,
    VK_FORMAT_A2B10G10R10_UINT_PACK32,                     // = 68,
    VK_FORMAT_A2B10G10R10_SINT_PACK32,                     // = 69,
    VK_FORMAT_R16_UNORM,                                   // = 70,
    VK_FORMAT_R16_SNORM,                                   // = 71,
    VK_FORMAT_R16_USCALED,                                 // = 72,
    VK_FORMAT_R16_SSCALED,                                 // = 73,
    VK_FORMAT_R16_UINT,                                    // = 74,
    VK_FORMAT_R16_SINT,                                    // = 75,
    VK_FORMAT_R16_SFLOAT,                                  // = 76,
    VK_FORMAT_R16G16_UNORM,                                // = 77,
    VK_FORMAT_R16G16_SNORM,                                // = 78,
    VK_FORMAT_R16G16_USCALED,                              // = 79,
    VK_FORMAT_R16G16_SSCALED,                              // = 80,
    VK_FORMAT_R16G16_UINT,                                 // = 81,
    VK_FORMAT_R16G16_SINT,                                 // = 82,
    VK_FORMAT_R16G16_SFLOAT,                               // = 83,
    VK_FORMAT_R16G16B16_UNORM,                             // = 84,
    VK_FORMAT_R16G16B16_SNORM,                             // = 85,
    VK_FORMAT_R16G16B16_USCALED,                           // = 86,
    VK_FORMAT_R16G16B16_SSCALED,                           // = 87,
    VK_FORMAT_R16G16B16_UINT,                              // = 88,
    VK_FORMAT_R16G16B16_SINT,                              // = 89,
    VK_FORMAT_R16G16B16_SFLOAT,                            // = 90,
    VK_FORMAT_R16G16B16A16_UNORM,                          // = 91,
    VK_FORMAT_R16G16B16A16_SNORM,                          // = 92,
    VK_FORMAT_R16G16B16A16_USCALED,                        // = 93,
    VK_FORMAT_R16G16B16A16_SSCALED,                        // = 94,
    VK_FORMAT_R16G16B16A16_UINT,                           // = 95,
    VK_FORMAT_R16G16B16A16_SINT,                           // = 96,
    VK_FORMAT_R16G16B16A16_SFLOAT,                         // = 97,
    VK_FORMAT_R32_UINT,                                    // = 98,
    VK_FORMAT_R32_SINT,                                    // = 99,
    VK_FORMAT_R32_SFLOAT,                                  // = 100,
    VK_FORMAT_R32G32_UINT,                                 // = 101,
    VK_FORMAT_R32G32_SINT,                                 // = 102,
    VK_FORMAT_R32G32_SFLOAT,                               // = 103,
    VK_FORMAT_R32G32B32_UINT,                              // = 104,
    VK_FORMAT_R32G32B32_SINT,                              // = 105,
    VK_FORMAT_R32G32B32_SFLOAT,                            // = 106,
    VK_FORMAT_R32G32B32A32_UINT,                           // = 107,
    VK_FORMAT_R32G32B32A32_SINT,                           // = 108,
    VK_FORMAT_R32G32B32A32_SFLOAT,                         // = 109,
    VK_FORMAT_R64_UINT,                                    // = 110,
    VK_FORMAT_R64_SINT,                                    // = 111,
    VK_FORMAT_R64_SFLOAT,                                  // = 112,
    VK_FORMAT_R64G64_UINT,                                 // = 113,
    VK_FORMAT_R64G64_SINT,                                 // = 114,
    VK_FORMAT_R64G64_SFLOAT,                               // = 115,
    VK_FORMAT_R64G64B64_UINT,                              // = 116,
    VK_FORMAT_R64G64B64_SINT,                              // = 117,
    VK_FORMAT_R64G64B64_SFLOAT,                            // = 118,
    VK_FORMAT_R64G64B64A64_UINT,                           // = 119,
    VK_FORMAT_R64G64B64A64_SINT,                           // = 120,
    VK_FORMAT_R64G64B64A64_SFLOAT,                         // = 121,
    VK_FORMAT_B10G11R11_UFLOAT_PACK32,                     // = 122,
    VK_FORMAT_E5B9G9R9_UFLOAT_PACK32,                      // = 123,
    VK_FORMAT_D16_UNORM,                                   // = 124,
    VK_FORMAT_X8_D24_UNORM_PACK32,                         // = 125,
    VK_FORMAT_D32_SFLOAT,                                  // = 126,
    VK_FORMAT_S8_UINT,                                     // = 127,
    VK_FORMAT_D16_UNORM_S8_UINT,                           // = 128,
    VK_FORMAT_D24_UNORM_S8_UINT,                           // = 129,
    VK_FORMAT_D32_SFLOAT_S8_UINT,                          // = 130,
    VK_FORMAT_BC1_RGB_UNORM_BLOCK,                         // = 131,
    VK_FORMAT_BC1_RGB_SRGB_BLOCK,                          // = 132,
    VK_FORMAT_BC1_RGBA_UNORM_BLOCK,                        // = 133,
    VK_FORMAT_BC1_RGBA_SRGB_BLOCK,                         // = 134,
    VK_FORMAT_BC2_UNORM_BLOCK,                             // = 135,
    VK_FORMAT_BC2_SRGB_BLOCK,                              // = 136,
    VK_FORMAT_BC3_UNORM_BLOCK,                             // = 137,
    VK_FORMAT_BC3_SRGB_BLOCK,                              // = 138,
    VK_FORMAT_BC4_UNORM_BLOCK,                             // = 139,
    VK_FORMAT_BC4_SNORM_BLOCK,                             // = 140,
    VK_FORMAT_BC5_UNORM_BLOCK,                             // = 141,
    VK_FORMAT_BC5_SNORM_BLOCK,                             // = 142,
    VK_FORMAT_BC6H_UFLOAT_BLOCK,                           // = 143,
    VK_FORMAT_BC6H_SFLOAT_BLOCK,                           // = 144,
    VK_FORMAT_BC7_UNORM_BLOCK,                             // = 145,
    VK_FORMAT_BC7_SRGB_BLOCK,                              // = 146,
    VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,                     // = 147,
    VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,                      // = 148,
    VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,                   // = 149,
    VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,                    // = 150,
    VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,                   // = 151,
    VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,                    // = 152,
    VK_FORMAT_EAC_R11_UNORM_BLOCK,                         // = 153,
    VK_FORMAT_EAC_R11_SNORM_BLOCK,                         // = 154,
    VK_FORMAT_EAC_R11G11_UNORM_BLOCK,                      // = 155,
    VK_FORMAT_EAC_R11G11_SNORM_BLOCK,                      // = 156,
    VK_FORMAT_ASTC_4x4_UNORM_BLOCK,                        // = 157,
    VK_FORMAT_ASTC_4x4_SRGB_BLOCK,                         // = 158,
    VK_FORMAT_ASTC_5x4_UNORM_BLOCK,                        // = 159,
    VK_FORMAT_ASTC_5x4_SRGB_BLOCK,                         // = 160,
    VK_FORMAT_ASTC_5x5_UNORM_BLOCK,                        // = 161,
    VK_FORMAT_ASTC_5x5_SRGB_BLOCK,                         // = 162,
    VK_FORMAT_ASTC_6x5_UNORM_BLOCK,                        // = 163,
    VK_FORMAT_ASTC_6x5_SRGB_BLOCK,                         // = 164,
    VK_FORMAT_ASTC_6x6_UNORM_BLOCK,                        // = 165,
    VK_FORMAT_ASTC_6x6_SRGB_BLOCK,                         // = 166,
    VK_FORMAT_ASTC_8x5_UNORM_BLOCK,                        // = 167,
    VK_FORMAT_ASTC_8x5_SRGB_BLOCK,                         // = 168,
    VK_FORMAT_ASTC_8x6_UNORM_BLOCK,                        // = 169,
    VK_FORMAT_ASTC_8x6_SRGB_BLOCK,                         // = 170,
    VK_FORMAT_ASTC_8x8_UNORM_BLOCK,                        // = 171,
    VK_FORMAT_ASTC_8x8_SRGB_BLOCK,                         // = 172,
    VK_FORMAT_ASTC_10x5_UNORM_BLOCK,                       // = 173,
    VK_FORMAT_ASTC_10x5_SRGB_BLOCK,                        // = 174,
    VK_FORMAT_ASTC_10x6_UNORM_BLOCK,                       // = 175,
    VK_FORMAT_ASTC_10x6_SRGB_BLOCK,                        // = 176,
    VK_FORMAT_ASTC_10x8_UNORM_BLOCK,                       // = 177,
    VK_FORMAT_ASTC_10x8_SRGB_BLOCK,                        // = 178,
    VK_FORMAT_ASTC_10x10_UNORM_BLOCK,                      // = 179,
    VK_FORMAT_ASTC_10x10_SRGB_BLOCK,                       // = 180,
    VK_FORMAT_ASTC_12x10_UNORM_BLOCK,                      // = 181,
    VK_FORMAT_ASTC_12x10_SRGB_BLOCK,                       // = 182,
    VK_FORMAT_ASTC_12x12_UNORM_BLOCK,                      // = 183,
    VK_FORMAT_ASTC_12x12_SRGB_BLOCK,                       // = 184,
    VK_FORMAT_G8B8G8R8_422_UNORM,                          // = 1000156000,
    VK_FORMAT_B8G8R8G8_422_UNORM,                          // = 1000156001,
    VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM,                   // = 1000156002,
    VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,                    // = 1000156003,
    VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM,                   // = 1000156004,
    VK_FORMAT_G8_B8R8_2PLANE_422_UNORM,                    // = 1000156005,
    VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM,                   // = 1000156006,
    VK_FORMAT_R10X6_UNORM_PACK16,                          // = 1000156007,
    VK_FORMAT_R10X6G10X6_UNORM_2PACK16,                    // = 1000156008,
    VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,          // = 1000156009,
    VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,      // = 1000156010,
    VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,      // = 1000156011,
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,  // = 1000156012,
    VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,   // = 1000156013,
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,  // = 1000156014,
    VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,   // = 1000156015,
    VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,  // = 1000156016,
    VK_FORMAT_R12X4_UNORM_PACK16,                          // = 1000156017,
    VK_FORMAT_R12X4G12X4_UNORM_2PACK16,                    // = 1000156018,
    VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,          // = 1000156019,
    VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,      // = 1000156020,
    VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,      // = 1000156021,
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,  // = 1000156022,
    VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,   // = 1000156023,
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,  // = 1000156024,
    VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,   // = 1000156025,
    VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,  // = 1000156026,
    VK_FORMAT_G16B16G16R16_422_UNORM,                      // = 1000156027,
    VK_FORMAT_B16G16R16G16_422_UNORM,                      // = 1000156028,
    VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM,                // = 1000156029,
    VK_FORMAT_G16_B16R16_2PLANE_420_UNORM,                 // = 1000156030,
    VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM,                // = 1000156031,
    VK_FORMAT_G16_B16R16_2PLANE_422_UNORM,                 // = 1000156032,
    VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM,                // = 1000156033,
    VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG,                 // = 1000054000,
    VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG,                 // = 1000054001,
    VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG,                 // = 1000054002,
    VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG,                 // = 1000054003,
    VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG,                  // = 1000054004,
    VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG,                  // = 1000054005,
    VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG,                  // = 1000054006,
    VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG,                  // = 1000054007,
    VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK_EXT,                   // = 1000066000,
    VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK_EXT,                   // = 1000066001,
    VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK_EXT,                   // = 1000066002,
    VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK_EXT,                   // = 1000066003,
    VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK_EXT,                   // = 1000066004,
    VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK_EXT,                   // = 1000066005,
    VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK_EXT,                   // = 1000066006,
    VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK_EXT,                   // = 1000066007,
    VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK_EXT,                  // = 1000066008,
    VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK_EXT,                  // = 1000066009,
    VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK_EXT,                  // = 1000066010,
    VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK_EXT,                 // = 1000066011,
    VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK_EXT,                 // = 1000066012,
    VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK_EXT,                 // = 1000066013,
                                                           // VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT,
                                                           // // = 1000340000,
                                                           // VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT,
                                                           // // = 1000340001,
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
    VK_PRESENT_MODE_MAILBOX_KHR,
    VK_PRESENT_MODE_FIFO_KHR,
    VK_PRESENT_MODE_FIFO_RELAXED_KHR,
    VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR,
    VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR,
};
constexpr VkBufferUsageFlagBits vkBufferUsageFlagBitArray[]{
    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,      VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
};
constexpr VkMemoryPropertyFlagBits vkMemoryPropertyFlagBitArray[]{
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,       VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,    VK_MEMORY_PROPERTY_PROTECTED_BIT,
    VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD, VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD,
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
    VK_IMAGE_CREATE_SPARSE_BINDING_BIT,
    VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT,
    VK_IMAGE_CREATE_SPARSE_ALIASED_BIT,
    VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    VK_IMAGE_CREATE_ALIAS_BIT,
    VK_IMAGE_CREATE_SPLIT_INSTANCE_BIND_REGIONS_BIT,
    VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT,
    VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT,
    VK_IMAGE_CREATE_EXTENDED_USAGE_BIT,
    VK_IMAGE_CREATE_PROTECTED_BIT,
    VK_IMAGE_CREATE_DISJOINT_BIT,
    VK_IMAGE_CREATE_CORNER_SAMPLED_BIT_NV,
    VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT,
    VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT,
};

constexpr VkImageType vkImageTypeArray[]{
    VK_IMAGE_TYPE_1D,
    VK_IMAGE_TYPE_2D,
    VK_IMAGE_TYPE_3D,
};
constexpr VkImageViewType vkImageViewTypeArray[]{
    VK_IMAGE_VIEW_TYPE_1D,       VK_IMAGE_VIEW_TYPE_2D,       VK_IMAGE_VIEW_TYPE_3D,         VK_IMAGE_VIEW_TYPE_CUBE,
    VK_IMAGE_VIEW_TYPE_1D_ARRAY, VK_IMAGE_VIEW_TYPE_2D_ARRAY, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY,
};
constexpr VkImageTiling vkImageTilingArray[]{VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_TILING_LINEAR};
constexpr VkImageUsageFlagBits vkImageUsageFlagBitArray[]{
    VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    VK_IMAGE_USAGE_SAMPLED_BIT,
    VK_IMAGE_USAGE_STORAGE_BIT,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
    VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV,
    VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT,
};
constexpr VkFormatFeatureFlagBits vkFormatFeatureFlagBitsArray[]{
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
    VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT, VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT,
    VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT, VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT,
    VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT,
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT,
    VK_FORMAT_FEATURE_BLIT_SRC_BIT, VK_FORMAT_FEATURE_BLIT_DST_BIT, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
    VK_FORMAT_FEATURE_TRANSFER_SRC_BIT, VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
    VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT, VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER_BIT,
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_SEPARATE_RECONSTRUCTION_FILTER_BIT,
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_BIT,
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_YCBCR_CONVERSION_CHROMA_RECONSTRUCTION_EXPLICIT_FORCEABLE_BIT,
    VK_FORMAT_FEATURE_DISJOINT_BIT, VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT,
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_MINMAX_BIT, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_CUBIC_BIT_IMG,
    // #ifdef VK_ENABLE_BETA_EXTENSIONS
    //		VK_FORMAT_FEATURE_VIDEO_DECODE_OUTPUT_BIT_KHR,
    // #endif
    // #ifdef VK_ENABLE_BETA_EXTENSIONS
    //		VK_FORMAT_FEATURE_VIDEO_DECODE_DPB_BIT_KHR,
    // #endif
    VK_FORMAT_FEATURE_ACCELERATION_STRUCTURE_VERTEX_BUFFER_BIT_KHR, VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT,
    // VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR,
    // #ifdef VK_ENABLE_BETA_EXTENSIONS
    //		VK_FORMAT_FEATURE_VIDEO_ENCODE_INPUT_BIT_KHR,
    // #endif
    // #ifdef VK_ENABLE_BETA_EXTENSIONS
    //		VK_FORMAT_FEATURE_VIDEO_ENCODE_DPB_BIT_KHR,
    // #endif
};
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
};
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
constexpr VkDescriptorType vkDescriptorTypeArray[]{VK_DESCRIPTOR_TYPE_SAMPLER,
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

constexpr VkSamplerMipmapMode vkSamplerMipmapModeArray[]{VK_SAMPLER_MIPMAP_MODE_NEAREST, VK_SAMPLER_MIPMAP_MODE_LINEAR};

constexpr VkSamplerAddressMode vkSamplerAddressModeArray[]{
    VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    //// Provided by VK_VERSION_1_2, VK_KHR_sampler_mirror_clamp_to_edge
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE ,
    // VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE_KHR,
};
constexpr VkCompareOp vkCompareOpArray[]{VK_COMPARE_OP_NEVER,
                                         VK_COMPARE_OP_LESS,
                                         VK_COMPARE_OP_EQUAL,
                                         VK_COMPARE_OP_LESS_OR_EQUAL,
                                         VK_COMPARE_OP_GREATER,
                                         VK_COMPARE_OP_NOT_EQUAL,
                                         VK_COMPARE_OP_GREATER_OR_EQUAL,
                                         VK_COMPARE_OP_ALWAYS};

constexpr VkAttachmentLoadOp vkAttachmentLoadOpArray[]{VK_ATTACHMENT_LOAD_OP_LOAD, VK_ATTACHMENT_LOAD_OP_CLEAR,
                                                       VK_ATTACHMENT_LOAD_OP_DONT_CARE, VK_ATTACHMENT_LOAD_OP_MAX_ENUM};
constexpr VkAttachmentStoreOp vkAttachmentStoreOpArray[]{VK_ATTACHMENT_STORE_OP_STORE, VK_ATTACHMENT_STORE_OP_DONT_CARE,
                                                         VK_ATTACHMENT_STORE_OP_NONE_QCOM,
                                                         VK_ATTACHMENT_STORE_OP_MAX_ENUM};
constexpr VkComponentSwizzle vkComponentSwizzleArray[]{
    VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_ZERO, VK_COMPONENT_SWIZZLE_ONE, VK_COMPONENT_SWIZZLE_R,
    VK_COMPONENT_SWIZZLE_G,        VK_COMPONENT_SWIZZLE_B,    VK_COMPONENT_SWIZZLE_A,   VK_COMPONENT_SWIZZLE_MAX_ENUM};
constexpr VkPipelineBindPoint vkPipelineBindPointArray[]{
    VK_PIPELINE_BIND_POINT_GRAPHICS, VK_PIPELINE_BIND_POINT_COMPUTE, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
    VK_PIPELINE_BIND_POINT_RAY_TRACING_NV, VK_PIPELINE_BIND_POINT_MAX_ENUM};
constexpr VkCommandBufferUsageFlagBits vkCommandBufferUsageFlagBitsArray[]{
    VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
    VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, VK_COMMAND_BUFFER_USAGE_FLAG_BITS_MAX_ENUM};
constexpr VkSubpassContents vkSubpassContentsArray[]{
    VK_SUBPASS_CONTENTS_INLINE,
    VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS,
};
constexpr VkIndexType vkIndexTypeArray[]{
    VK_INDEX_TYPE_NONE_KHR,   // Provided by VK_KHR_ray_tracing
    VK_INDEX_TYPE_UINT8_EXT,  // Provided by VK_EXT_index_type_uint8
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
    VK_LOGIC_OP_CLEAR,         VK_LOGIC_OP_AND,         VK_LOGIC_OP_AND_REVERSE, VK_LOGIC_OP_COPY,
    VK_LOGIC_OP_AND_INVERTED,  VK_LOGIC_OP_NO_OP,       VK_LOGIC_OP_XOR,         VK_LOGIC_OP_OR,
    VK_LOGIC_OP_NOR,           VK_LOGIC_OP_EQUIVALENT,  VK_LOGIC_OP_INVERT,      VK_LOGIC_OP_OR_REVERSE,
    VK_LOGIC_OP_COPY_INVERTED, VK_LOGIC_OP_OR_INVERTED, VK_LOGIC_OP_NAND,        VK_LOGIC_OP_SET,
};
constexpr VkBlendFactor vkBlendFactorArray[]{VK_BLEND_FACTOR_ZERO,
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
    VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT,
    VK_ERROR_NOT_PERMITTED_EXT,
    VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT,
    VK_THREAD_IDLE_KHR,
    VK_THREAD_DONE_KHR,
    VK_OPERATION_DEFERRED_KHR,
    VK_OPERATION_NOT_DEFERRED_KHR,
    VK_PIPELINE_COMPILE_REQUIRED_EXT,
};
constexpr VkFenceCreateFlagBits VkFenceCreateFlagBitsArray[]{
    VK_FENCE_CREATE_SIGNALED_BIT,
};
constexpr VkSemaphoreType vkSemaphoreTypeArray[]{
    VK_SEMAPHORE_TYPE_BINARY,
    VK_SEMAPHORE_TYPE_TIMELINE,
};
constexpr VkBorderColor vkBorderColorArray[]{
    VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK, VK_BORDER_COLOR_INT_TRANSPARENT_BLACK, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
    VK_BORDER_COLOR_INT_OPAQUE_BLACK,        VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,    VK_BORDER_COLOR_INT_OPAQUE_WHITE,
    VK_BORDER_COLOR_FLOAT_CUSTOM_EXT,        VK_BORDER_COLOR_INT_CUSTOM_EXT,
};
constexpr VkPipelineStageFlagBits vkPipelineStageFlagBitsArray[]{
    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
    VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    VK_PIPELINE_STAGE_TRANSFER_BIT,
    VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
    VK_PIPELINE_STAGE_HOST_BIT,
    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
    VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT,
    VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT,
    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
    VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
    VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV,
    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV,
    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV,
    VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT,
    VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV,
};
constexpr VkDependencyFlagBits vkDependencyFlagBitsArray[]{
    VK_DEPENDENCY_BY_REGION_BIT,
    VK_DEPENDENCY_DEVICE_GROUP_BIT,
    VK_DEPENDENCY_VIEW_LOCAL_BIT,
};
constexpr VkAccessFlagBits vkAccessFlagBitsArray[]{
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT,
    VK_ACCESS_INDEX_READ_BIT,
    VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
    VK_ACCESS_UNIFORM_READ_BIT,
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
    VK_ACCESS_SHADER_READ_BIT,
    VK_ACCESS_SHADER_WRITE_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    VK_ACCESS_TRANSFER_READ_BIT,
    VK_ACCESS_TRANSFER_WRITE_BIT,
    VK_ACCESS_HOST_READ_BIT,
    VK_ACCESS_HOST_WRITE_BIT,
    VK_ACCESS_MEMORY_READ_BIT,
    VK_ACCESS_MEMORY_WRITE_BIT,
    VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT,
    VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT,
    VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT,
    VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT,
    VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT,
    VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR,
    VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
    VK_ACCESS_SHADING_RATE_IMAGE_READ_BIT_NV,
    VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT,
    VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV,
    VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV,
};
constexpr VkDebugReportFlagBitsEXT vkDebugReportFlagBitsEXTArray[]{
    VK_DEBUG_REPORT_INFORMATION_BIT_EXT,          // = 0x00000001,
    VK_DEBUG_REPORT_WARNING_BIT_EXT,              // = 0x00000002,
    VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,  // = 0x00000004,
    VK_DEBUG_REPORT_ERROR_BIT_EXT,                // = 0x00000008,
    VK_DEBUG_REPORT_DEBUG_BIT_EXT,                // = 0x00000010,
    VK_DEBUG_REPORT_FLAG_BITS_MAX_ENUM_EXT,       // = 0x7FFFFFFF
};
constexpr VkDebugUtilsMessageSeverityFlagBitsEXT vkDebugUtilsMessageSeverityFlagBitsEXTArray[]{
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT,         // = 0x00000001,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,            // = 0x00000010,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,         // = 0x00000100,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,           // = 0x00001000,
    VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT,  // = 0x7FFFFFFF
};
constexpr VkPipelineCreateFlagBits vkPipelineCreateFlagBits[]{
    VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,                          // 0x00000001,
    VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT,                             // 0x00000002,
    VK_PIPELINE_CREATE_DERIVATIVE_BIT,                                    // 0x00000004,
    VK_PIPELINE_CREATE_VIEW_INDEX_FROM_DEVICE_INDEX_BIT,                  // 0x00000008,
    VK_PIPELINE_CREATE_DISPATCH_BASE_BIT,                                 // 0x00000010,
    VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR,       // 0x00004000,
    VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR,   // 0x00008000,
    VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR,          // 0x00010000,
    VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_INTERSECTION_SHADERS_BIT_KHR,  // 0x00020000,
    VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR,                // 0x00001000,
    VK_PIPELINE_CREATE_RAY_TRACING_SKIP_AABBS_BIT_KHR,                    // 0x00002000,
    VK_PIPELINE_CREATE_DEFER_COMPILE_BIT_NV,                              // 0x00000020,
    VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR,                        // 0x00000040,
    VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR,          // 0x00000080,
    VK_PIPELINE_CREATE_INDIRECT_BINDABLE_BIT_NV,                          // 0x00040000,
    VK_PIPELINE_CREATE_LIBRARY_BIT_KHR,                                   // 0x00000800,
    VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_EXT,         // 0x00000100,
    VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT_EXT,                   // 0x00000200,
};
VkPipelineCreateFlags Map(PipelineCreateFlagBits flags) { return static_cast<VkPipelineCreateFlags>(flags); }
VkAccessFlags Map(AccessFlagBits flags) { return static_cast<VkAccessFlags>(flags); }
VkDependencyFlags Map(DependencyFlagBits flags) { return static_cast<VkDependencyFlags>(flags); }
VkPipelineStageFlags Map(PipelineStageFlagBits flags) { return static_cast<VkPipelineStageFlags>(flags); }
VkBorderColor Map(BorderColor color) { return vkBorderColorArray[static_cast<size_t>(color)]; }
VkSemaphoreType Map(SemaphoreType type) { return vkSemaphoreTypeArray[static_cast<size_t>(type)]; }
VkFenceCreateFlags Map(FenceCreateFlagBits flags) {
    VkFenceCreateFlags ret{};
    int a = static_cast<int>(flags);
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= VkFenceCreateFlagBitsArray[i];
    }
    return ret;
}
VkDynamicState Map(DynamicState state) { return vkDynamicStateArray[static_cast<size_t>(state)]; }
VkColorComponentFlags Map(ColorComponentFlagBits flag) { return static_cast<VkColorComponentFlags>(flag); }
VkBlendOp Map(BlendOp op) { return vkBlendOpArray[static_cast<size_t>(op)]; }
VkBlendFactor Map(BlendFactor factor) { return vkBlendFactorArray[static_cast<size_t>(factor)]; }
VkLogicOp Map(LogicOp op) { return vkLogicOpArray[static_cast<size_t>(op)]; }
VkStencilOp Map(StencilOp op) { return vkStencilOpArray[static_cast<size_t>(op)]; }
VkFrontFace Map(FrontFace face) { return vkFrontFaceArray[static_cast<size_t>(face)]; }
VkCullModeFlags Map(CullMode cullmode) {
    return static_cast<VkCullModeFlags>(vkCullModeFlagBitsArray[static_cast<size_t>(cullmode)]);
}
VkPolygonMode Map(PolygonMode mode) { return vkPolygonModeArray[static_cast<size_t>(mode)]; }
VkPrimitiveTopology Map(PrimitiveTopology topology) { return vkPrimitiveTopologyArray[static_cast<size_t>(topology)]; }

VkCommandBufferResetFlags Map(CommandBufferResetFlatBits flag) {
    VkCommandBufferResetFlags ret{};
    int a = static_cast<int>(flag);
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkCommandBufferResetFlagBitsArray[i];
    }
    return ret;
}
VkIndexType Map(IndexType type) { return vkIndexTypeArray[static_cast<size_t>(type)]; }
VkSubpassContents Map(SubpassContents contents) { return vkSubpassContentsArray[static_cast<size_t>(contents)]; }

VkPipelineBindPoint Map(PipelineBindPoint bindPoint) {
    return vkPipelineBindPointArray[static_cast<size_t>(bindPoint)];
}
VkComponentSwizzle Map(ComponentSwizzle swizzle) { return vkComponentSwizzleArray[static_cast<size_t>(swizzle)]; }
VkAttachmentLoadOp Map(AttachmentLoadOp op) { return vkAttachmentLoadOpArray[static_cast<size_t>(op)]; }
VkAttachmentStoreOp Map(AttachmentStoreOp op) { return vkAttachmentStoreOpArray[static_cast<size_t>(op)]; }
VkCompareOp Map(CompareOp op) { return vkCompareOpArray[static_cast<size_t>(op)]; }
VkSamplerAddressMode Map(SamplerWrapMode mode) { return vkSamplerAddressModeArray[static_cast<size_t>(mode)]; }
VkSamplerMipmapMode Map(SamplerMipmapMode mode) { return vkSamplerMipmapModeArray[static_cast<size_t>(mode)]; }
VkShaderStageFlagBits Map(ShaderStageFlagBits flag) { return static_cast<VkShaderStageFlagBits>(flag); }
VkDescriptorType Map(DescriptorType type) { return vkDescriptorTypeArray[static_cast<size_t>(type)]; }

VkFilter Map(Filter filter) { return vkFilterArray[static_cast<size_t>(filter)]; }
VkCommandPoolCreateFlags Map(CommandPoolCreateFlagBits flag) {
    VkCommandPoolCreateFlags ret{};
    int a = static_cast<int>(flag);
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkCommandPoolCreateFlagBitArray[i];
    }
    return ret;
}

VkImageCreateFlags Map(ImageCreateFlagBits flags) { return static_cast<VkImageCreateFlags>(flags) & 0x0fffffff; }
VkImageType Map(ImageType type) { return vkImageTypeArray[static_cast<size_t>(type)]; }
VkImageViewType Map(ImageViewType type) { return vkImageViewTypeArray[static_cast<size_t>(type)]; }
VkImageTiling Map(ImageTiling tiling) { return vkImageTilingArray[static_cast<size_t>(tiling)]; }
VkImageUsageFlags Map(ImageUsageFlagBits flag) {
    VkImageUsageFlags ret{};
    int a = static_cast<int>(flag);
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkImageUsageFlagBitArray[i];
    }
    return ret;
}
VkImageLayout Map(ImageLayout layout) { return vkImageLayoutArray[static_cast<size_t>(layout)]; }

VkBufferUsageFlags Map(BufferUsageFlagBits flag) {
    VkBufferUsageFlags ret{};
    int a = static_cast<int>(flag);
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkBufferUsageFlagBitArray[i];
    }
    return ret;
}

VkMemoryPropertyFlags Map(MemoryPropertyFlagBits flag) {
    return static_cast<VkMemoryPropertyFlags>(flag);
    // VkMemoryPropertyFlags ret{};
    // int a = static_cast<int>(flag);
    // for (int i = 0; a > 0 && i < 32; ++i, a >>= 1)
    //{
    //	if (a & 1)
    //		ret |= vkMemoryPropertyFlagBitArray[i];
    // }
    // return ret;
}

VkFormat Map(Format format) { return vkFormatArray[static_cast<size_t>(format)]; }
Format Map(VkFormat format) {
    static std::unordered_map<VkFormat, Format> tempMap;
    if (tempMap.find(format) == tempMap.end()) {
        for (int i = 0; i < static_cast<int>(Format::Num); ++i) {
            auto a = Map(static_cast<Format>(i));
            if (a == format) return tempMap[format] = static_cast<Format>(i);
        }
        ST_THROW("failed to find corresponding ShitFormat:", format, ",use RGBA8_UNORM:", VK_FORMAT_R8G8B8A8_UNORM);
    } else
        return tempMap[format];
}

VkColorSpaceKHR Map(ColorSpace colorSpace) { return vkColorSpaceArray[static_cast<size_t>(colorSpace)]; }
ColorSpace Map(VkColorSpaceKHR colorSpace) {
    static std::unordered_map<VkColorSpaceKHR, ColorSpace> tempMap;
    if (tempMap.find(colorSpace) == tempMap.end()) {
        for (int i = 0, len = static_cast<int>(Format::Num); i < len; ++i) {
            auto a = Map(static_cast<ColorSpace>(i));
            if (a == colorSpace) return tempMap[a] = static_cast<ColorSpace>(i);
        }
        ST_THROW("failed to find corresponding ShitFormat");
    } else
        return tempMap[colorSpace];
}

VkPresentModeKHR Map(PresentMode mode) { return vkPresentModeArray[static_cast<size_t>(mode)]; }
PresentMode Map(VkPresentModeKHR mode) {
    for (int i = 0, len = static_cast<int>(PresentMode::Num); i < len; ++i) {
        if (Map(static_cast<PresentMode>(i)) == mode) return static_cast<PresentMode>(i);
    }
    ST_THROW("failed to find suitable present mode");
}
VkCommandBufferLevel Map(CommandBufferLevel level) {
    if (level == CommandBufferLevel::SECONDARY)
        return VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    else
        return VK_COMMAND_BUFFER_LEVEL_PRIMARY;
}
VkCommandBufferUsageFlags Map(CommandBufferUsageFlagBits usage) {
    VkCommandBufferUsageFlags ret{};
    if (static_cast<bool>(usage & CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT))
        ret |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (static_cast<bool>(usage & CommandBufferUsageFlagBits::RENDER_PASS_CONTINUE_BIT))
        ret |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    if (static_cast<bool>(usage & CommandBufferUsageFlagBits::SIMULTANEOUS_USE_BIT))
        ret |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    return ret;
}
VkQueueFlags Map(QueueFlagBits flag) {
    int a = static_cast<int>(flag);
    VkQueueFlags ret{};
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkQueueFlagBitArray[i];
    }
    return ret;
}
QueueFlagBits Map(VkQueueFlags flag) {
    QueueFlagBits ret{};
    if (flag & VK_QUEUE_GRAPHICS_BIT) ret |= Shit::QueueFlagBits::GRAPHICS_BIT;
    if (flag & VK_QUEUE_TRANSFER_BIT) ret |= Shit::QueueFlagBits::TRANSFER_BIT;
    if (flag & VK_QUEUE_COMPUTE_BIT) ret |= Shit::QueueFlagBits::COMPUTE_BIT;
    if (flag & VK_QUEUE_SPARSE_BINDING_BIT) ret |= Shit::QueueFlagBits::SPARSE_BINDING_BIT;
    return ret;
}
VkFormatFeatureFlags Map(FormatFeatureFlagBits flags) {
    int a = static_cast<int>(flags);
    VkFormatFeatureFlags ret{};
    for (int i = 0; a > 0 && i < 32; ++i, a >>= 1) {
        if (a & 1) ret |= vkFormatFeatureFlagBitsArray[i];
    }
    return ret;
}
VkImageAspectFlags Map(ImageAspectFlagBits flags) { return static_cast<VkImageAspectFlags>(flags); }
VkStencilFaceFlags Map(StencilFaceFlagBits flags) { return static_cast<VkStencilFaceFlags>(flags); }
Result Map(VkResult res) {
    switch (res) {
        case VK_SUCCESS:
            return Result::SUCCESS;
        case VK_TIMEOUT:
            return Result::TIMEOUT;
        case VK_NOT_READY:
            return Result::NOT_READY;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Result::SHIT_ERROR_OUT_OF_DATE;
        case VK_SUBOPTIMAL_KHR:
            return Result::SUBOPTIMAL;
        default:
            return Result::SHIT_ERROR;
    }
}
}  // namespace Shit
