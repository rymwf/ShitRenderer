/**
 * @file GLFence.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitFence.h>
#include "GLPrerequisites.h"
namespace Shit
{
	class GLFence final : public Fence
	{
		GLsync mHandle;

	public:
		GLFence(const FenceCreateInfo &createInfo) : Fence(createInfo)
		{
			mHandle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
		}
		virtual ~GLFence() override
		{
			glDeleteSync(mHandle);
		}
	};
}