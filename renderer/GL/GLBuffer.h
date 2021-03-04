/**
 * @file GLBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBuffer.h>
#include "GLPrerequisites.h"
namespace Shit
{
	class GLBuffer final : public Buffer
	{
	public:
		GLBuffer(const BufferCreateInfo &createInfo) : Buffer(createInfo)
		{
		}
	};
} // namespace Shit


