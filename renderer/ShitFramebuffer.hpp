/**
 * @file ShitFramebuffer.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit
{
	class Framebuffer
	{
	protected:
		FramebufferCreateInfo mCreateInfo;
		Framebuffer(const FramebufferCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~Framebuffer() {}
		constexpr const FramebufferCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
