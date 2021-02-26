/**
 * @file GLContext.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "GLContext.h"
#include <renderer/ShitWindow.h>

namespace Shit
{
	void GLContext::QueryGLExtensionNames(std::vector<const GLubyte *> &extensionNames)
	{
		int extensionCount{};
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		extensionNames.resize(extensionCount);
		LOG_VAR(extensionCount);
		for (int i = 0; i < extensionCount; ++i)
		{
			extensionNames[i] = glGetStringi(GL_EXTENSIONS, i);
			LOG_VAR(extensionNames[i]);
		}
	}

#if _WIN32

	void GLContextWin32::CreateRenderContext()
	{
		const int attribList[] =
			{
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				//WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, //or WGL_TYPE_COLORINDEX_ARB
				WGL_COLOR_BITS_ARB, 32,
				WGL_DEPTH_BITS_ARB, 24,
				WGL_STENCIL_BITS_ARB, 8,
				0, // End
			};

		int pixelFormat{};
		UINT numFormats;
		if (!(mHDeviceContext, attribList, NULL, 1, &pixelFormat, &numFormats))
		{
			THROW("wglChoosePixelFormatARB failed");
		}
		LOG_VAR(pixelFormat);

		int majorversion = max((static_cast<int>(mRenderSystemCreateInfo->version) >> 2) & 1, 1);
		int minorversion = (static_cast<int>(mRenderSystemCreateInfo->version) >> 1) & 1;

		int contexFlag{};
		if (static_cast<bool>(mRenderSystemCreateInfo->flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
			contexFlag |= WGL_CONTEXT_DEBUG_BIT_ARB;
		if (static_cast<bool>(mRenderSystemCreateInfo->flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_FORWARD_COMPATIBLE_BIT))
			contexFlag |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		int profileMaskFlag{WGL_CONTEXT_CORE_PROFILE_BIT_ARB};
		if (static_cast<bool>(mRenderSystemCreateInfo->flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
			profileMaskFlag |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;

		const int attribList2[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,
			majorversion, //seems to be not support < 330
			WGL_CONTEXT_MINOR_VERSION_ARB,
			minorversion,
			WGL_CONTEXT_FLAGS_ARB, contexFlag,
			WGL_CONTEXT_PROFILE_MASK_ARB,
			profileMaskFlag,
			0};
		wglDeleteContext(mHRenderContext);
		mHRenderContext = wglCreateContextAttribsARB(mHDeviceContext, 0, attribList2);
		if (!mHRenderContext)
		{
			THROW("failed to create render context");
		}
		if (!wglMakeContextCurrentARB(mHDeviceContext, mHDeviceContext, mHRenderContext))
		{
			fprintf(stderr, "ErrorCode: (0x)%x/n", GetLastError());
			THROW("failed to make context current");
		}

		//reinit opengl and wgl
		LOADGL
		LOADWGL

		LOG_VAR(QueryInstanceExtensionNames());
	}

	GLContextWin32::GLContextWin32(const RenderSystemCreateInfo *pRenderSystemCreateInfo, const ContextCreateInfo &createInfo)
		: GLContext(pRenderSystemCreateInfo, createInfo)
	{
		mHDeviceContext = GetDC(static_cast<HWND>(createInfo.pWindow->GetNativeHandle()));
		if (!mHDeviceContext)
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
		int iPixelFormat = ChoosePixelFormat(mHDeviceContext, &pfd);
		LOG_VAR(iPixelFormat);
		SetPixelFormat(mHDeviceContext, iPixelFormat, &pfd);
		mHRenderContext = wglCreateContext(mHDeviceContext);
		if (!mHRenderContext)
			THROW("failed to create surface");

		wglMakeCurrent(mHDeviceContext, mHRenderContext);

		LOADGL
		LOADWGL
	}

	const char *GLContextWin32::QueryInstanceExtensionNames()
	{
		if (wglIsExtensionSupported("WGL_ARB_extensions_string"))
			return wglGetExtensionsStringARB(mHDeviceContext);
		else if (wglIsExtensionSupported("WGL_EXT_extensions_string"))
			return wglGetExtensionsStringEXT();
		THROW("failed to query instance extension names");
	}
#endif
}
