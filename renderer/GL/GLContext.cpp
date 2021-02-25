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

	GLContext::GLContext(const ContextCreateInfo &createInfo)
	{
#ifdef _WIN32
		mHDeviceContext = GetDC(static_cast<HWND>(createInfo.pWindow->GetNativeHandle()));
		if (!mHDeviceContext)
			throw std::runtime_error("failed to create surface");
#endif

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
		SetPixelFormat(mHDeviceContext, iPixelFormat, &pfd);
		mHRenderContext = wglCreateContext(mHDeviceContext);
		if (!mHRenderContext)
			throw std::runtime_error("failed to create surface");

		wglMakeCurrent(mHDeviceContext, mHRenderContext);

		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			/* Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			throw std::runtime_error("failed to init glew");
		}
		err = wglewInit();
		if (GLEW_OK != err)
		{
			/* Problem: glewInit failed, something is seriously wrong. */
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
			throw std::runtime_error("failed to init wglew");
		}

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

		int pixelFormat;
		UINT numFormats;
		if (!wglChoosePixelFormatARB(mHDeviceContext, attribList, NULL, 1, &pixelFormat, &numFormats))
		{
			throw std::runtime_error("wglChoosePixelFormatARB failed");
		}




		LOG(QueryInstanceExtensionNames());
		std::vector<const GLubyte*> a;
		QueryGLExtensionNames(a);
		LOG_VAR(glGetString(GL_VENDOR));
		LOG_VAR(glGetString(GL_RENDERER));
		LOG_VAR(glGetString(GL_VERSION));
		LOG_VAR(glGetString(GL_SHADING_LANGUAGE_VERSION));
	}

	const char *GLContext::QueryInstanceExtensionNames()
	{
		if (wglewIsSupported("WGL_ARB_extensions_string"))
			return wglGetExtensionsStringARB(mHDeviceContext);
		else if (wglewIsSupported("WGL_EXT_extensions_string"))
			return wglGetExtensionsStringEXT();
		throw std::runtime_error("failed to query instance extension names");
	}
	void GLContext::QueryGLExtensionNames(std::vector<const GLubyte *> &extensionNames)
	{
		int extensionCount;
		glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
		extensionNames.resize(extensionCount);
		LOG_VAR(extensionCount);
		for (int i = 0; i < extensionCount; ++i)
		{
			extensionNames[i] = glGetStringi(GL_EXTENSIONS, i);
			LOG_VAR(extensionNames[i]);
		}
	}
}
