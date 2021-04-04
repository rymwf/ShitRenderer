/**
 * @file ShitBuffer.hpp
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
	class Buffer
	{
	protected:
		BufferCreateInfo mCreateInfo;

	public:
		Buffer(const BufferCreateInfo &createInfo) : mCreateInfo(createInfo)
		{
		}
		constexpr const BufferCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
		virtual ~Buffer() {}
		virtual void MapBuffer(uint64_t offset, uint64_t size, void **ppData) = 0;
		virtual void UnMapBuffer() = 0;
	};
} // namespace Shit
