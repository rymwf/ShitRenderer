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

#ifdef _WIN32
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
	using ProgramHandle = GLuint;

	struct ShitGLVersion
	{
		int major;
		int minor;
	};
	extern ShitGLVersion GLVersion;

	struct ProgramCreateInfo
	{
		std::shared_ptr<std::vector<GLuint>> pShaders;
		bool separable;
		bool retrievable;
	};

	namespace GL
	{
		void queryGLExtensionNames(std::vector<const GLubyte *> &extensionNames);

		void querySupportedShaderBinaryFormat(std::vector<GLint> &shaderBinaryFormats);
		void querySupportedProgramBinaryFormat(std::vector<GLint> &programBinaryFormats);
		bool isSupportShaderBinaryFormat(GLenum format);

#ifdef _WIN32
		const char *queryWGLExtensionNames(HDC hdc);
#endif
		void listGLInfo();
	}

	GLenum MapInternalFormat(ShitFormat format);
	GLenum Map(ShaderStageFlagBits flag);
	GLenum Map(BufferUsageFlagBits flag);
	GLenum Map(BufferMutableStorageUsage usage);
	GLbitfield Map(BufferMapFlagBits flag);
	GLbitfield Map(BufferStorageFlagBits flag);

	ProgramHandle CreateProgram(const ProgramCreateInfo &createInfo);

} // namespace Shit
