/**
 * @file VKRenderPass.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderPass.hpp>
#include "VKPrerequisites.hpp"
namespace Shit
{
	class VKRenderPass final : public RenderPass
	{
		VkDevice mDevice;
		VkRenderPass mHandle;

	public:
		VKRenderPass(VkDevice device, const RenderPassCreateInfo &createInfo);
		~VKRenderPass() override
		{
			vkDestroyRenderPass(mDevice, mHandle, nullptr);
		}
		constexpr VkRenderPass GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
