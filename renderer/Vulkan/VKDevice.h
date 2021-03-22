/**
 * @file VKDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDevice.h>
#include "VKPrerequisites.h"

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

		void ExecuteOneTimeCommands(const std::function<void(CommandBuffer *)> &func);

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

		CommandPool *Create(const CommandPoolCreateInfo &createInfo) override;

		Queue *Create(const QueueCreateInfo &createInfo) override;

		Buffer *Create(const BufferCreateInfo &createInfo, void *pData) override;

		Image *Create(const ImageCreateInfo &createInfo, void *pData) override;

		ImageView *Create(const ImageViewCreateInfo &createInfo) override;

		DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) override;

		PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) override;

		RenderPass *Create(const RenderPassCreateInfo &createInfo) override;

		Framebuffer *Create(const FramebufferCreateInfo &createInfo) override;

		Semaphore *Create(const SemaphoreCreateInfo &createInfo) override;

		Fence *Create(const FenceCreateInfo &createInfo) override;
	};

} // namespace Shit
