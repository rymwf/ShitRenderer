/**
 * @file GLDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLDevice.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{
#ifdef _WIN32
	GLDeviceWin32::GLDeviceWin32(ShitWindow *pWindow)
	{
		mHDC = GetDC(static_cast<WindowWin32 *>(pWindow)->GetHWND());
		if (!mHDC)
			THROW("failed to create surface");

		PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
			1,							   // version number
			PFD_DRAW_TO_WINDOW |		   // support window
				PFD_SUPPORT_OPENGL |	   // support OpenGL
				PFD_DOUBLEBUFFER,		   // double buffered
			PFD_TYPE_RGBA,				   // RGBA type
			32,							   // 32-bit color depth framebuffer bits
			0, 0, 0, 0, 0, 0,			   // color bits ignored
			0,							   // no alpha buffer
			0,							   // shift bit ignored
			0,							   // no accumulation buffer
			0, 0, 0, 0,					   // accum bits ignored
			24,							   // 24-bit z-buffer
			8,							   // 8-bit stencil buffer
			0,							   // no auxiliary buffer
			PFD_MAIN_PLANE,				   // main layer
			0,							   // reserved
			0, 0, 0						   // layer masks ignored
		};
		// get the best available match of pixel format for the device context
		int iPixelFormat = ChoosePixelFormat(mHDC, &pfd);
		LOG_VAR(iPixelFormat);
		SetPixelFormat(mHDC, iPixelFormat, &pfd);
		mHRenderContext = wglCreateContext(mHDC);
		if (!mHRenderContext)
			THROW("failed to create surface");

		wglMakeCurrent(mHDC, mHRenderContext);

		LOADGL
		LOADWGL
	}
#endif
}