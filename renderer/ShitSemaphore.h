/**
 * @file ShitSemaphore.h
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
	class Semaphore
	{
	protected:
		SemaphoreCreateInfo mCreateInfo;
		Semaphore(const SemaphoreCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~Semaphore() {}
	};
} // namespace Shit
