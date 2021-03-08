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
		void CreateCommandBuffers(const CommandBufferCreateInfo &createInfo, std::vector<CommandBuffer *> &commandBuffers) override
		{
			commandBuffers.resize(createInfo.count);
			for(uint32_t i=0;i<createInfo.count;++i)
			{
				mCommandBuffers.emplace_back(std::make_unique<GLCommandBuffer>(createInfo));
				commandBuffers.emplace_back(mCommandBuffers.back().get());
			}
		}
	};

} // namespace Shit
