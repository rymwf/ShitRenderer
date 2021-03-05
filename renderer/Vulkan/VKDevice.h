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
#include "VKSwapchain.h"
#include "VKShader.h"
#include "VKCommandPool.h"
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKQueue.h"
#include "VKBuffer.h"
#include "VKImage.h"

namespace Shit
{

	class VKDevice final:public Device
	{
		VkDevice mDevice;
		VkPhysicalDevice mPhysicalDevice;

		std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;

		std::optional<uint32_t> mGraphicQueueFamilyIndex;
		std::optional<uint32_t> mTransferQueueFamilyIndex;

	public:
		VKDevice(PhysicalDevice physicalDevice);
		~VKDevice() override
		{
		}
		constexpr VkDevice GetHandle()
		{
			return mDevice;
		}

		std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex(ShitWindow *pWindow) override;

		constexpr VkPhysicalDevice GetPhysicalDevice()const
		{
			return mPhysicalDevice;
		}

		std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(QueueFlagBits flag, const std::unordered_set<uint32_t> &skipIndices) override;

		Swapchain* CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;
		void DestroyShader(Shader *pShader) override;

		GraphicsPipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		CommandPool *CreateCommandPool(const CommandPoolCreateInfo &createInfo) override;
		void DestroyCommandPool(CommandPool *commandPool) override;

		CommandBuffer *CreateCommandBuffer(const CommandBufferCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Result WaitForFence(Fence *fence, uint64_t timeout) override;

		Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) override;
	};

} // namespace Shit
