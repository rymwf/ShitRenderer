/**
 * @file GLSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLSwapchain.h"
namespace Shit
{
#ifdef _WIN32
	GLSwapchainWin32::GLSwapchainWin32(
		HDC hdc,
		const SwapchainCreateInfo &createInfo,
		RendererVersion version,
		RenderSystemCreateFlagBits flags)
		: Swapchain(createInfo), mHdc(hdc)
	{
		//device->MakeCurrent();

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
				WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
				0, // End
			};

		int pixelFormat{};
		UINT numFormats;
		if (!wglChoosePixelFormatARB(mHdc, attribList, NULL, 1, &pixelFormat, &numFormats))
		{
			THROW("wglChoosePixelFormatARB failed");
		}
		LOG_VAR(pixelFormat);

		int majorversion = max((static_cast<int>(version) >> 2) & 1, 1);
		int minorversion = (static_cast<int>(version) >> 1) & 1;

		int contexFlag{};
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
			contexFlag |= WGL_CONTEXT_DEBUG_BIT_ARB;
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_FORWARD_COMPATIBLE_BIT))
			contexFlag |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		int profileMaskFlag{WGL_CONTEXT_CORE_PROFILE_BIT_ARB};
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
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

		mHglrc = wglCreateContextAttribsARB(mHdc, 0, attribList2);
		if (!mHglrc)
		{
			THROW("failed to create render context");
		}

		MakeCurrent();

		if (createInfo.presentMode == PresentMode::IMMEDIATE)
			wglSwapIntervalEXT(0);
		else if (createInfo.presentMode == PresentMode::FIFO)
			wglSwapIntervalEXT(1);

		//reinit opengl and wgl
		LOADGL
		LOADWGL

		//
		glGetIntegerv(GL_MAJOR_VERSION, &GLVersion.major);
		glGetIntegerv(GL_MINOR_VERSION, &GLVersion.minor);

		LOG_VAR(GLVersion.major);
		LOG_VAR(GLVersion.minor);

		LOG_VAR(GL::queryWGLExtensionNames(mHdc));
#ifndef NDEBUG
		GL::listGLInfo();
#endif
	}
#endif
} // namespace Shit
