/**
 * @file ShitCommandBuffer.h
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
	class CommandBuffer
	{
	protected:
		CommandBufferCreateInfo mCreateInfo;

		CommandBuffer(const CommandBufferCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~CommandBuffer() {}
	};
}