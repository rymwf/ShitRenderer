/**
 * @file VKFramebuffer.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitFramebuffer.hpp>
#include "VKPrerequisites.hpp"
namespace Shit
{
	class VKFramebuffer final : public Framebuffer
	{
		VkDevice mDevice;
		VkFramebuffer mHandle;

	public:
		VKFramebuffer(VkDevice device, const FramebufferCreateInfo &createInfo);
		~VKFramebuffer() override
		{
			vkDestroyFramebuffer(mDevice, mHandle, nullptr);
		}
		constexpr VkFramebuffer GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shi
