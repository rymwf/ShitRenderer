/**
 * @file VKPrerequisites.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <algorithm>

#include "config.hpp"
#include "renderer/ShitRendererPrerequisites.hpp"

// #define VK_NO_PROTOTYPES

#include <vulkan/vulkan.h>

#define VK_LAYER_KHRONOS_validation "VK_LAYER_KHRONOS_validation"
#define VK_LAYER_LUNARG_monitor "VK_LAYER_LUNARG_monitor"
#define VK_LAYER_LUNARG_device_simulation "VK_LAYER_LUNARG_device_simulation"

#define CHECK_VK_RESULT(x)                                               \
    {                                                                    \
        auto res = x;                                                    \
        if (res != VK_SUCCESS) ST_THROW(#x " failed, error code:", res); \
    }

namespace Shit {
class VKDevice;
class VKRenderSystem;
class VKCommandBuffer;

extern VKRenderSystem *g_RenderSystem;

namespace VK {
int rateDeviceSuitability(VkPhysicalDevice device, VkSurfaceKHR surface = VK_NULL_HANDLE);
void queryPhysicalDeviceGroupInfo(VkInstance instance,
                                  std::vector<VkPhysicalDeviceGroupProperties> &physicalDeviceGroupProperties);

/**
 * @brief
 *
 * @param queueFamilyProperties
 * @param flag
 * @return std::optional<uint32_t>
 */
std::optional<uint32_t> findQueueFamilyIndexByFlag(std::span<const VkQueueFamilyProperties> queueFamilyProperties,
                                                   VkQueueFlags flag);

std::optional<uint32_t> findQueueFamilyIndexPresent(VkPhysicalDevice physicalDevice, uint32_t familyNum,
                                                    VkSurfaceKHR surface);
VkPhysicalDevice pickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface = VK_NULL_HANDLE);
void querySurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                         std::vector<VkSurfaceFormatKHR> &surfaceFormats);
void querySurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                              std::vector<VkPresentModeKHR> &presentModes);
void queryDisplayProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPropertiesKHR> &displayProperties);
void queryDisplayPlaneProperties(VkPhysicalDevice physicalDevice,
                                 std::vector<VkDisplayPlanePropertiesKHR> &displayPlaneProperties);
void queryDisplayModeProperties(VkPhysicalDevice physicalDevice, VkDisplayKHR display,
                                std::vector<VkDisplayModePropertiesKHR> &displayModeProperties);
uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeIndexFilter,
                             VkMemoryPropertyFlags properties);
VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex);
void createCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count, VkCommandBufferLevel level,
                          std::vector<VkCommandBuffer> &commandBuffers);
VkCommandBuffer beginOneTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool);
void endOneTimeCommands(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool,
                        VkCommandBuffer commandBuffer);
VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, std::span<const VkFormat> candidates,
                             VkImageTiling tiling, VkFormatFeatureFlags features);
void generateMipmaps(VkCommandBuffer commandBuffer, VkImage image, int32_t width, int32_t height, uint32_t mipLevels,
                     VkFilter filter);
VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
void queryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties);
void queryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties);
void queryPhysicalDevices(VkInstance instance, std::vector<PhysicalDevice> &physicalDevices);
void queryDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                    std::vector<VkExtensionProperties> &extensionProperies);
VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex);
}  // namespace VK

VkAccessFlags Map(AccessFlagBits flags);
VkDependencyFlags Map(DependencyFlagBits flags);
VkPipelineStageFlags Map(PipelineStageFlagBits flags);
VkBorderColor Map(BorderColor color);
VkSemaphoreType Map(SemaphoreType type);
VkFenceCreateFlags Map(FenceCreateFlagBits flags);
VkDynamicState Map(DynamicState state);
VkColorComponentFlags Map(ColorComponentFlagBits flag);
VkBlendOp Map(BlendOp op);
VkBlendFactor Map(BlendFactor factor);
VkLogicOp Map(LogicOp op);
VkStencilOp Map(StencilOp op);
VkFrontFace Map(FrontFace face);
VkCullModeFlags Map(CullMode cullmode);
VkPolygonMode Map(PolygonMode mode);
VkPrimitiveTopology Map(PrimitiveTopology topology);
VkCommandBufferResetFlags Map(CommandBufferResetFlatBits flag);
VkIndexType Map(IndexType type);
VkSubpassContents Map(SubpassContents contents);
VkPipelineBindPoint Map(PipelineBindPoint bindPoint);
VkComponentSwizzle Map(ComponentSwizzle swizzle);
VkAttachmentLoadOp Map(AttachmentLoadOp op);
VkAttachmentStoreOp Map(AttachmentStoreOp op);
VkCompareOp Map(CompareOp op);
VkSamplerAddressMode Map(SamplerWrapMode mode);
VkSamplerMipmapMode Map(SamplerMipmapMode mode);
VkShaderStageFlagBits Map(ShaderStageFlagBits flag);
VkDescriptorType Map(DescriptorType type);
VkFilter Map(Filter filter);
VkCommandPoolCreateFlags Map(CommandPoolCreateFlagBits flag);
VkImageCreateFlags Map(ImageCreateFlagBits flag);
VkImageType Map(ImageType type);
VkImageViewType Map(ImageViewType type);
VkImageTiling Map(ImageTiling tiling);
VkImageUsageFlags Map(ImageUsageFlagBits flag);
VkImageLayout Map(ImageLayout layout);
VkBufferUsageFlags Map(BufferUsageFlagBits flag);
VkMemoryPropertyFlags Map(MemoryPropertyFlagBits flag);
VkFormat Map(Format format);
Format Map(VkFormat format);
VkColorSpaceKHR Map(ColorSpace colorSpace);
ColorSpace Map(VkColorSpaceKHR colorSpace);
VkPresentModeKHR Map(PresentMode mode);
PresentMode Map(VkPresentModeKHR mode);
VkCommandBufferLevel Map(CommandBufferLevel level);
VkCommandBufferUsageFlags Map(CommandBufferUsageFlagBits usage);
VkQueueFlags Map(QueueFlagBits flag);
QueueFlagBits Map(VkQueueFlags flag);
VkFormatFeatureFlags Map(FormatFeatureFlagBits flags);
VkPipelineCreateFlags Map(PipelineCreateFlagBits flags);
VkImageAspectFlags Map(ImageAspectFlagBits flags);
VkStencilFaceFlags Map(StencilFaceFlagBits flags);

Result Map(VkResult res);
}  // namespace Shit
