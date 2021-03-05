/**
 * @file ShitWindow.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitWindow.h"
#include "ShitRenderSystem.h"
namespace Shit
{
	Surface *ShitWindow::CreateSurface(const SurfaceCreateInfo &createInfo)
	{
		mpSurface = mpRenderSystem->CreateSurface(createInfo, this);
		return mpSurface.get();
	}

} // namespace Shit
