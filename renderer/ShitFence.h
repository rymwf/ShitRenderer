/**
 * @file ShitFence.h
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
	class Fence
	{
	protected:
		FenceCreateInfo mCreateInfo;
		Fence(const FenceCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~Fence() {}
	};
} // namespace Shit
