/**
 * @file GLDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitDevice.h>
#include "GLPrerequisites.h"

namespace Shit
{

#ifdef _WIN32
	class GLDeviceWin32 final : public Device
	{
		HDC mHDC;
		HGLRC mHRenderContext; //false context

	public:
		GLDeviceWin32(ShitWindow *pWindow);
		~GLDeviceWin32() override
		{
			wglDeleteContext(mHRenderContext);
		}
		void MakeCurrent() const
		{
			wglMakeCurrent(mHDC, mHRenderContext);
		}
		HDC GetHDC() const
		{
			return mHDC;
		}
	};
#endif

}