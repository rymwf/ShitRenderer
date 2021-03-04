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

		CommandPool(const CommandPoolCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~CommandPool() {}
	};
} // namespace Shit