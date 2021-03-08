/**
 * @file VKPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRendererPrerequisites.h>
#include <algorithm>

#include <vulkan/vulkan.h>
#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

constexpr char *LAYER_VALIDATION_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";

namespace Shit
{
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
		void createBuffer(VkPhysicalDevice physicalDevice, VkDevice logicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer &outBuffer, VkDeviceMemory &outBufferMemory);
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

	VkSubpassContents Map(SubpassContents contents);
	VkPipelineBindPoint Map(PipelineBindPoint bindPoint);
	VkComponentSwizzle Map(ComponentSwizzle swizzle);
	VkAttachmentLoadOp Map(AttachmentLoadOp op);
	VkAttachmentStoreOp Map(AttachmentStoreOp op);
	VkCompareOp Map(CompareOp op);
	VkSamplerAddressMode Map(SamplerWrapMode mode);
	VkSamplerMipmapMode Map(SamplerMipmapMode mode);
	VkShaderStageFlags Map(ShaderStageFlagBits flag);
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
	VkColorSpaceKHR Map(ColorSpace colorSpace);
	VkPresentModeKHR Map(PresentMode mode);
	VkCommandBufferLevel Map(CommandBufferLevel level);
	VkQueueFlags Map(QueueFlagBits flag);
	VkImageAspectFlags GetImageAspectFromFormat(ShitFormat format);
}
