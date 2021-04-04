/**
 * @file VKCommandPool.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandPool.hpp>
#include "VKPrerequisites.hpp"
#include "VKCommandBuffer.hpp"
namespace Shit
{
	class VKCommandPool final : public CommandPool
	{
		VkCommandPool mHandle;
		VkDevice mDevice;

	public:
		VKCommandPool(VkDevice device, const CommandPoolCreateInfo &createInfo) : CommandPool(createInfo), mDevice(device)
		{
			VkCommandPoolCreateInfo commandPoolCreateInfo{
				VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
				nullptr,
				Map(mCreateInfo.flags),
				mCreateInfo.queueFamilyIndex.index};
			if (vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mHandle) != VK_SUCCESS)
				THROW("failed to create command pool");
		}
		constexpr VkCommandPool GetHandle() const
		{
			return mHandle;
		}
		~VKCommandPool() override
		{
			vkDestroyCommandPool(mDevice, mHandle, nullptr);
		}

		void CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, std::vector<CommandBuffer *> &commandBuffers) override
		{
			commandBuffers.resize(createInfo.count);
			for (uint32_t i = 0; i < createInfo.count; ++i)
			{
				mCommandBuffers.emplace_back(std::make_unique<VKCommandBuffer>(mDevice, mHandle, createInfo));
				commandBuffers[i] = mCommandBuffers.back().get();
			}
		}
	};
} // namespace Shit
