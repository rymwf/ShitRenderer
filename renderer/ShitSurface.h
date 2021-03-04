/**
 * @file ShitSurface.h
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
	class Surface
	{
	protected:
		SurfaceCreateInfo mCreateInfo;

	public:
		Surface(const SurfaceCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		virtual ~Surface() {}
	};
} // namespace Shit
