/**
 * @file ShitFramebuffer.h
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
	class Framebuffer
	{
	protected:
		FramebufferCreateInfo mCreateInfo;
		Framebuffer(const FramebufferCreateInfo &createInfo) {}

	public:
		virtual ~Framebuffer() {}
		constexpr const FramebufferCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
