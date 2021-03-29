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
#include <renderer/ShitCommandPool.hpp>
#include "GLPrerequisites.hpp"
#include "GLCommandBuffer.hpp"

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
	};

} // namespace Shit
