/**
 * @file ShitBufferView.hpp
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
	class BufferView
	{
	protected:
		BufferViewCreateInfo mCreateInfo;

	public:
		BufferView(const BufferViewCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~BufferView() {}
		constexpr const BufferViewCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};

} // namespace Shit
