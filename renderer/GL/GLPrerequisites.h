/**
 * @file GLPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include <GL/glew.h>

#if _WIN32
#include <GL/wglew.h>
#endif

#include <renderer/ShitRendererPrerequisites.h>

#define LOADGL                                                            \
	{                                                                     \
		GLenum err = glewInit();                                          \
		if (GLEW_OK != err)                                               \
		{                                                                 \
			/* Problem: glewInit failed, something is seriously wrong. */ \
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));      \
			THROW("failed to init glew");                                 \
		}                                                                 \
	}

#define LOADWGL                                                           \
	{                                                                     \
		GLenum err = wglewInit();                                         \
		if (GLEW_OK != err)                                               \
		{                                                                 \
			/* Problem: glewInit failed, something is seriously wrong. */ \
			fprintf(stderr, "Error: %s\n", glewGetErrorString(err));      \
			THROW("failed to init wglew");                                \
		}                                                                 \
	}

#define glIsExtensionSupported(x) glewIsExtensionSupported(x)
#define wglIsExtensionSupported(x) wglewIsSupported(x)

namespace Shit
{
} // namespace Shit
