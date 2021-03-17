/**
 * @file GLCommandPool.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandPool.h>
#include "GLPrerequisites.h"
#include "GLCommandBuffer.h"

namespace Shit
{
	class GLCommandPool final : public CommandPool
	{
		GLStateManager *mpStateManager;

	public:
		GLCommandPool(GLStateManager *pStateManger, const CommandPoolCreateInfo &createInfo)
			: CommandPool(createInfo), mpStateManager(pStateManger) {}
		~GLCommandPool() override {}
		void CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, std::vector<CommandBuffer *> &commandBuffers) override;
		void DestroyCommandBuffer(CommandBuffer *pCommandBuffer) override;
	};

} // namespace Shit
