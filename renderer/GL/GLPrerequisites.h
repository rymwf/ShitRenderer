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
#include "GLState.h"

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

#define MAX_TEXTURE_IMAGE_UNITS 80 //!< The number of texture units is implementation dependent, but must be at least 80, the value can be get from GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define BUFFER_TARGET_NUM 15

namespace Shit
{
	class GLDevice;

	struct ShitGLVersion
	{
		int major;
		int minor;
	};

	extern ShitGLVersion GLVersion;


	extern bool SHIT_GL_110;
	extern bool SHIT_GL_120;
	extern bool SHIT_GL_121;
	extern bool SHIT_GL_130;
	extern bool SHIT_GL_140;
	extern bool SHIT_GL_150;
	extern bool SHIT_GL_200;
	extern bool SHIT_GL_210;
	extern bool SHIT_GL_300;
	extern bool SHIT_GL_310;
	extern bool SHIT_GL_320;
	extern bool SHIT_GL_330;
	extern bool SHIT_GL_400;
	extern bool SHIT_GL_410;
	extern bool SHIT_GL_420;
	extern bool SHIT_GL_430;
	extern bool SHIT_GL_440;
	extern bool SHIT_GL_450;
	extern bool SHIT_GL_460;


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

	const std::unordered_map<GLenum, int> glCapabilityTable;

	GLenum Map(BlendOp op);
	GLenum Map(BlendFactor factor);
	GLenum Map(LogicOp op);
	GLenum Map(StencilOp op);
	GLenum Map(FrontFace face);
	GLenum Map(CullMode mode);
	GLenum Map(PolygonMode mode);
	GLenum Map(PrimitiveTopology topology);
	GLenum Map(ComponentSwizzle swizzle);
	GLenum MapInternalFormat(ShitFormat format);
	GLenum MapExternalFormat(ShitFormat format);
	GLenum Map(ShaderStageFlagBits flag);
	GLbitfield MapShaderStageFlags(ShaderStageFlagBits flags);
	GLenum Map(BufferUsageFlagBits flag);
	GLenum Map(BufferMutableStorageUsage usage);
	GLbitfield Map(BufferMapFlagBits flag);
	GLbitfield Map(BufferStorageFlagBits flag);
	GLenum Map(SamplerWrapMode wrapMode);
	GLenum Map(CompareOp op);
	GLenum Map(Filter filter);
	GLenum Map(ImageType imageType, SampleCountFlagBits sampleCountFlag);
	GLenum Map(ImageViewType viewType, SampleCountFlagBits sampleCountFlag);
	GLenum Map(DataType dataType);
	GLenum Map(IndexType type);

} // namespace Shit
