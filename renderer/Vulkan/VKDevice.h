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

	class VKDevice final:public Device
	{
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;

	public:
		VKDevice(PhysicalDevice physicalDevice);
		~VKDevice() override
		{
		}
		constexpr VkDevice GetHandle()
		{
			return mDevice;
		}

		void ExecuteOneTimeCommands(const std::function<void(CommandBuffer*)> &func);

		std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex(ShitWindow *pWindow) override;

		constexpr VkPhysicalDevice GetPhysicalDevice()const
		{
			return mPhysicalDevice;
		}

		std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices) override;

		Swapchain* CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;

		Pipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		CommandPool *CreateCommandPool(const CommandPoolCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Result WaitForFence(Fence *fence, uint64_t timeout) override;

		Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) override;

		Image *CreateImage(const ImageCreateInfo &createInfo, void *pData) override;

		ImageView *CreateImageView(const ImageViewCreateInfo &createInfo) override;

		DescriptorSetLayout *CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) override;

		PipelineLayout *CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo) override;

		RenderPass *CreateRenderPass(const RenderPassCreateInfo &createInfo) override;

		Framebuffer *CreateFramebuffer(const FramebufferCreateInfo &createInfo) override;

		Semaphore *CreateDeviceSemaphore(const SemaphoreCreateInfo &createInfo) override;

		Fence *CreateFence(const FenceCreateInfo &createInfo) override;
	};

} // namespace Shit
