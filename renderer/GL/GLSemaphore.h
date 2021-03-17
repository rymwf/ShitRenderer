/**
 * @file GLSemaphore.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSemaphore.h>
#include "GLPrerequisites.h"
#include "GLFence.h"
namespace Shit
{
	class GLSemaphore final : public Semaphore, public GLFence
	{
	public:
		GLSemaphore(GLStateManager *pStateManger, const SemaphoreCreateInfo &createInfo)
			: Semaphore(createInfo), GLFence(pStateManger, {})
		{
		}
	};
}
