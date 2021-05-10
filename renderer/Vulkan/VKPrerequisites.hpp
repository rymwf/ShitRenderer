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
#include <renderer/ShitRendererPrerequisites.hpp>
#include <algorithm>

#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

#define VK_LAYER_KHRONOS_validation "VK_LAYER_KHRONOS_validation"
#define VK_LAYER_LUNARG_monitor "VK_LAYER_LUNARG_monitor"
#define VK_LAYER_LUNARG_device_simulation "VK_LAYER_LUNARG_device_simulation"

#define CHECK_VK_RESULT(x)                             \
	{                                                  \
		auto res = x;                                  \
		if (res != VK_SUCCESS)                         \
			THROW(#x " failed" + std::to_string(res)); \
	}

namespace Shit
{
	class VKDevice;

	extern VkInstance vk_instance;

	static inline void destroyVkSurface(VkSurfaceKHR surface)
	{
		vkDestroySurfaceKHR(vk_instance, surface, nullptr);
	}

	namespace VK
	{
		int rateDeviceSuitability(VkPhysicalDevice device);
		void queryQueueFamilyProperties(VkPhysicalDevice physicalDevice, std::vector<VkQueueFamilyProperties> &queueFamilyProperties);
		void queryPhysicalDeviceGroupInfo(VkInstance instance, std::vector<VkPhysicalDeviceGroupProperties> &physicalDeviceGroupProperties);

		std::optional<uint32_t> findQueueFamilyIndexByFlag(std::vector<VkQueueFamilyProperties> &queueFamilyProperties, VkQueueFlags flag, const std::unordered_set<uint32_t> &skipIndices);
		std::optional<uint32_t> findQueueFamilyIndexPresent(VkPhysicalDevice physicalDevice, uint32_t familyNum, VkSurfaceKHR surface);
		VkPhysicalDevice pickPhysicalDevice(VkInstance instance);
		void querySurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &surfaceFormats);
		void querySurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &presentModes);
		VkShaderModule createShaderModule(VkDevice logicalDevice, const std::string &code);
		VkDescriptorSetLayout createDescriptorSetLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayoutBinding> &setLayoutBindings);
		VkPipelineLayout createPipelineLayout(VkDevice logicalDevice, const std::vector<VkDescriptorSetLayout> &setLayouts, const std::vector<VkPushConstantRange> &pushConstantRanges);
		VkCommandPool createCommandPool(VkDevice logicalDevice, uint32_t queueFamilyIndex);
		VkSemaphore createSemaphore(VkDevice logicalDevice);
		void queryDisplayProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPropertiesKHR> &displayProperties);
		void queryDisplayPlaneProperties(VkPhysicalDevice physicalDevice, std::vector<VkDisplayPlanePropertiesKHR> &displayPlaneProperties);
		void queryDisplayModeProperties(VkPhysicalDevice physicalDevice, VkDisplayKHR display, std::vector<VkDisplayModePropertiesKHR> &displayModeProperties);
		VkFence createFence(VkDevice logicalDevice, VkFenceCreateFlags flags);
		uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t typeIndexFilter, VkMemoryPropertyFlags properties);
		VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex);
		void createCommandBuffers(VkDevice logicalDevice, VkCommandPool commandPool, uint32_t count, VkCommandBufferLevel level, std::vector<VkCommandBuffer> &commandBuffers);
		VkDescriptorPool createDescriptorPool(VkDevice logicalDevice, uint32_t maxSetCount, const std::vector<VkDescriptorPoolSize> &descriptorPoolSizes);
		void createDescriptorSets(VkDevice logicalDevice, VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts, std::vector<VkDescriptorSet> &descriptorSets);
		void createImage(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, const VkImageCreateInfo &imageCreateInfo, VkMemoryPropertyFlags properties, VkImage &outImage, VkDeviceMemory &outImageMemory);
		VkCommandBuffer beginOneTimeCommands(VkDevice logicalDevice, VkCommandPool commandPool);
		void endOneTimeCommands(VkDevice logicalDevice, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
		VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		void generateMipmaps(VkCommandBuffer commandBuffer, VkImage image, int32_t width, int32_t height, uint32_t mipLevels, VkFilter filter);
		VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
		void queryInstanceExtensionProperties(const char *layerName, std::vector<VkExtensionProperties> &extensionProperties);
		void queryInstanceLayerProperties(std::vector<VkLayerProperties> &layerProperties);
		void queryPhysicalDevices(VkInstance instance, std::vector<PhysicalDevice> &physicalDevices);
		void queryDeviceExtensionProperties(VkPhysicalDevice physicalDevice, std::vector<VkExtensionProperties> &extensionProperies);
		VkDeviceMemory allocateMemory(VkDevice logicalDevice, VkDeviceSize memsize, uint32_t memoryTypeIndex);
	}

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
	VkFormat Map(ShitFormat format);
	ShitFormat Map(VkFormat format);
	VkColorSpaceKHR Map(ColorSpace colorSpace);
	ColorSpace Map(VkColorSpaceKHR colorSpace);
	VkPresentModeKHR Map(PresentMode mode);
	PresentMode Map(VkPresentModeKHR mode);
	VkCommandBufferLevel Map(CommandBufferLevel level);
	VkQueueFlags Map(QueueFlagBits flag);
	VkImageAspectFlags GetImageAspectFromFormat(ShitFormat format);
	VkFormat GetFormat(DataType dataType, uint32_t components, bool normalized);
}
