/**
 * @file ShitBuffer.h
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
	class Buffer
	{
		BufferCreateInfo mCreateInfo;

	public:
		Buffer(const BufferCreateInfo &createInfo) : mCreateInfo(createInfo)
		{
		}

	};
} // namespace Shit
