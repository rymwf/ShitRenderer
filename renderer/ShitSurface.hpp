/**
 * @file ShitSurface.hpp
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
	class Surface
	{
	protected:
		SurfaceCreateInfo mCreateInfo;

	public:
		Surface(const SurfaceCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Surface() {}
		constexpr const SurfaceCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};
} // namespace Shit
