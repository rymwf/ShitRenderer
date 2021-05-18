/**
 * @file VKDevice.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDevice.hpp>
#include "VKPrerequisites.hpp"

namespace Shit
{

	class VKDevice final : public Device
	{
		VkDevice mDevice;

		std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;

	public:
		VKDevice(const DeviceCreateInfo &createInfo);
		~VKDevice() override
		{
		}
		constexpr VkDevice GetHandle()
		{
			return mDevice;
		}

		PFN_vkVoidFunction GetDeviceProcAddr(const char *pName);

		void LoadDeviceExtensionFunctions();

		Result WaitIdle() override;

		std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex(ShitWindow *pWindow) override;

		constexpr VkPhysicalDevice GetPhysicalDevice() const
		{
			return static_cast<VkPhysicalDevice>(std::get<PhysicalDevice>(mCreateInfo.physicalDevice));
		}

		std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices) override;

		void GetWindowPixelFormats(const ShitWindow *pWindow, std::vector<WindowPixelFormat> &format) override;

		void GetPresentModes(const ShitWindow *pWindow, std::vector<PresentMode> &presentModes) override;

		Swapchain *Create(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;

		Shader *Create(const ShaderCreateInfo &createInfo) override;

		Pipeline *Create(const GraphicsPipelineCreateInfo &createInfo) override;

		Pipeline *Create(const ComputePipelineCreateInfo &createInfo) override;

		CommandPool *Create(const CommandPoolCreateInfo &createInfo) override;

		Queue *Create(const QueueCreateInfo &createInfo) override;

		Buffer *Create(const BufferCreateInfo &createInfo, const void *pData) override;

		Image *Create(const ImageCreateInfo &createInfo, const void *pData) override;

		ImageView *Create(const ImageViewCreateInfo &createInfo) override;

		DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) override;

		PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) override;

		RenderPass *Create(const RenderPassCreateInfo &createInfo) override;

		Framebuffer *Create(const FramebufferCreateInfo &createInfo) override;

		Semaphore *Create(const SemaphoreCreateInfo &createInfo) override;

		Fence *Create(const FenceCreateInfo &createInfo) override;

		Sampler *Create(const SamplerCreateInfo &createInfo) override;

		DescriptorPool *Create(const DescriptorPoolCreateInfo &createInfo) override;

		void UpdateDescriptorSets(const std::vector<WriteDescriptorSet> &descriptorWrites, const std::vector<CopyDescriptorSet> &descriptorCopies) override;
	};

} // namespace Shit
