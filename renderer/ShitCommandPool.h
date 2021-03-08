/**
 * @file ShitCommandPool.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
namespace Shit
{
	class CommandPool
	{
	protected:
		CommandPoolCreateInfo mCreateInfo;
		std::vector<std::unique_ptr<CommandBuffer>> mCommandBuffers;

		CommandPool(const CommandPoolCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~CommandPool() {}

		virtual void CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, std::vector<CommandBuffer *> &commandBuffers) = 0;
		virtual void DestroyCommandBuffer(CommandBuffer *pCommandBuffer) = 0;

		constexpr const CommandPoolCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit