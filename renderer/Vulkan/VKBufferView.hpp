/**
 * @file VKBufferView.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBufferView.hpp>
#include "VKPrerequisites.hpp"
namespace Shit
{
	class VKBufferView final : public BufferView
	{
		VkDevice mDevice;
		VkBufferView mHandle;

	public:
		VKBufferView(VkDevice device, const BufferViewCreateInfo &createInfo);
		~VKBufferView() override
		{
			vkDestroyBufferView(mDevice, mHandle, nullptr);
		}
		constexpr VkBufferView GetHandle() const
		{
			return mHandle;
		}
	};
} // namespace Shit
