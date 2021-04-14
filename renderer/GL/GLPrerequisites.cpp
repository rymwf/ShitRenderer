/**
 * @file GLPrerequisites.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLPrerequisites.hpp"

namespace Shit
{
	ShitGLVersion GLVersion;

	/**
	 * @brief if current gpu do not support a version, the value will be false,
	 * they will be initialized after creating a device;
	 */
	bool SHIT_GL_110;
	bool SHIT_GL_120;
	bool SHIT_GL_121;
	bool SHIT_GL_130;
	bool SHIT_GL_140;
	bool SHIT_GL_150;
	bool SHIT_GL_200;
	bool SHIT_GL_210;
	bool SHIT_GL_300;
	bool SHIT_GL_310;
	bool SHIT_GL_320;
	bool SHIT_GL_330;
	bool SHIT_GL_400;
	bool SHIT_GL_410;
	bool SHIT_GL_420;
	bool SHIT_GL_430;
	bool SHIT_GL_440;
	bool SHIT_GL_450;
	bool SHIT_GL_460;

	namespace GL
	{
		void queryGLExtensionNames(std::vector<const GLubyte *> &extensionNames)
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

		void querySupportedShaderBinaryFormat(std::vector<GLint> &shaderBinaryFormats)
		{
			GLint count;
			glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &count);
			shaderBinaryFormats.resize(count);
			glGetIntegerv(GL_SHADER_BINARY_FORMATS, shaderBinaryFormats.data());
		}

		void querySupportedProgramBinaryFormat(std::vector<GLint> &programBinaryFormats)
		{
			GLint count;
			glGetIntegerv(GL_NUM_PROGRAM_BINARY_FORMATS, &count);
			programBinaryFormats.resize(count);
			glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, programBinaryFormats.data());
		}

		bool isSupportShaderBinaryFormat(GLenum format)
		{
			std::vector<GLint> shaderBinaryFormats;
			querySupportedShaderBinaryFormat(shaderBinaryFormats);
			for (auto &shaderBinaryFormat : shaderBinaryFormats)
			{
				LOG_VAR(shaderBinaryFormat);
				if (static_cast<GLenum>(shaderBinaryFormat) == format)
					return true;
			}
			return false;
		}

		void listGLInfo()
		{
			// check opengl informations
			LOG_VAR(glGetString(GL_VENDOR));
			LOG_VAR(glGetString(GL_RENDERER));
			LOG_VAR(glGetString(GL_VERSION));
			LOG_VAR(glGetString(GL_SHADING_LANGUAGE_VERSION));
			int numShadingLanguageVersions{};
			glGetIntegerv(GL_NUM_SHADING_LANGUAGE_VERSIONS, &numShadingLanguageVersions);
			LOG_VAR(numShadingLanguageVersions);
			for (int i = 0; i < numShadingLanguageVersions; ++i)
				LOG_VAR(glGetStringi(GL_SHADING_LANGUAGE_VERSION, i));

			int samples;
			glGetIntegerv(GL_SAMPLES, &samples);
			LOG_VAR(samples);

			int maxFramebufferWidth{}, maxFramebufferHeight{};
			glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &maxFramebufferWidth);
			glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &maxFramebufferHeight);
			LOG_VAR(maxFramebufferWidth);
			LOG_VAR(maxFramebufferHeight);

			if (GLVersion.major * 10 + GLVersion.minor >= 43)
			{
				//framebuffer parameter
				GLenum list0[]{
					//		GL_FRAMEBUFFER_DEFAULT_WIDTH,
					//		GL_FRAMEBUFFER_DEFAULT_HEIGHT,
					//		GL_FRAMEBUFFER_DEFAULT_LAYERS,
					//		GL_FRAMEBUFFER_DEFAULT_SAMPLES,
					//		GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS,

					GL_DOUBLEBUFFER,
					GL_IMPLEMENTATION_COLOR_READ_FORMAT,
					GL_IMPLEMENTATION_COLOR_READ_TYPE,
					GL_SAMPLES,
					GL_SAMPLE_BUFFERS,
					GL_STEREO};
				const char *list1[]{
					"GL_DOUBLEBUFFER",
					"GL_IMPLEMENTATION_COLOR_READ_FORMAT",
					"GL_IMPLEMENTATION_COLOR_READ_TYPE",
					"GL_SAMPLES",
					"GL_SAMPLE_BUFFERS",
					"GL_STEREO"};

				//default framebuffer
				for (int i = 0, len = sizeof list0 / sizeof list0[0]; i < len; ++i)
				{
					GLint a{};
					glGetFramebufferParameteriv(GL_FRAMEBUFFER, list0[i], &a);
					LOG(list1[i]);
					LOG(a);
				}
			}

			GLenum list2[]{
				GL_FRONT_LEFT,
				GL_FRONT_RIGHT,
				GL_BACK_LEFT,
				GL_BACK_RIGHT,
				GL_DEPTH,
				GL_STENCIL};
			const char *list3[]{
				"GL_FRONT_LEFT",
				"GL_FRONT_RIGHT",
				"GL_BACK_LEFT",
				"GL_BACK_RIGHT",
				"GL_DEPTH",
				"GL_STENCIL"};
			GLenum list4[]{
				GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
				GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
				GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING};
			const char *list5[]{
				"GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE",
				"GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE",
				"GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING"};
			for (int i = 0; i < 6; ++i)
			{
				GLint a{};
				glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, list2[i], GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &a);
				LOG(list3[i]);
				LOG(a);
				if (a != GL_NONE)
				{
					for (int j = 0, len = sizeof list4 / sizeof list4[0]; j < len; ++j)
					{
						glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, list2[i], list4[j], &a);
						LOG(list5[j]);
						LOG(a);
					}
				}
			}

			// extensions
#if 0
	GLint a;
	glGetIntegerv(GL_NUM_EXTENSIONS, &a);
	for (GLint i = 0; i < a; ++i)
		LOG(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
#endif
			constexpr GLenum glCapabilityArray[] = {
				GL_BLEND,
				//GL_CLIP_DISTANCE,
				GL_COLOR_LOGIC_OP,
				GL_CULL_FACE,
				GL_DEBUG_OUTPUT,
				GL_DEBUG_OUTPUT_SYNCHRONOUS,
				GL_DEPTH_CLAMP,
				GL_DEPTH_TEST,
				GL_DITHER,
				GL_FRAMEBUFFER_SRGB,
				GL_LINE_SMOOTH,
				GL_MULTISAMPLE,
				GL_POLYGON_OFFSET_FILL,
				GL_POLYGON_OFFSET_LINE,
				GL_POLYGON_OFFSET_POINT,
				GL_POLYGON_SMOOTH,
				GL_PRIMITIVE_RESTART,
				GL_PRIMITIVE_RESTART_FIXED_INDEX,
				GL_RASTERIZER_DISCARD,
				GL_SAMPLE_ALPHA_TO_COVERAGE,
				GL_SAMPLE_ALPHA_TO_ONE,
				GL_SAMPLE_COVERAGE,
				GL_SAMPLE_SHADING,
				GL_SAMPLE_MASK,
				GL_SCISSOR_TEST,
				GL_STENCIL_TEST,
				GL_TEXTURE_CUBE_MAP_SEAMLESS,
				GL_PROGRAM_POINT_SIZE,
			};
			const char *glCapabilityEnumNames[]{
				"GL_BLEND",
				"GL_COLOR_LOGIC_OP",
				"GL_CULL_FACE",
				"GL_DEBUG_OUTPUT",
				"GL_DEBUG_OUTPUT_SYNCHRONOUS",
				"GL_DEPTH_CLAMP",
				"GL_DEPTH_TEST",
				"GL_DITHER",
				"GL_FRAMEBUFFER_SRGB",
				"GL_LINE_SMOOTH",
				"GL_MULTISAMPLE",
				"GL_POLYGON_OFFSET_FILL",
				"GL_POLYGON_OFFSET_LINE",
				"GL_POLYGON_OFFSET_POINT",
				"GL_POLYGON_SMOOTH",
				"GL_PRIMITIVE_RESTART",
				"GL_PRIMITIVE_RESTART_FIXED_INDEX",
				"GL_RASTERIZER_DISCARD",
				"GL_SAMPLE_ALPHA_TO_COVERAGE",
				"GL_SAMPLE_ALPHA_TO_ONE",
				"GL_SAMPLE_COVERAGE",
				"GL_SAMPLE_SHADING",
				"GL_SAMPLE_MASK",
				"GL_SCISSOR_TEST",
				"GL_STENCIL_TEST",
				"GL_TEXTURE_CUBE_MAP_SEAMLESS",
				"GL_PROGRAM_POINT_SIZE",
			};

			for (int i = 0, len = sizeof(glCapabilityArray) / sizeof(glCapabilityArray[0]); i < len; ++i)
			{
				LOG(glCapabilityEnumNames[i]);
				LOG(bool(glIsEnabled(glCapabilityArray[i])));
			}
		}

	}
	constexpr GLenum glDataTypeArray[]{
		GL_BYTE,
		GL_UNSIGNED_BYTE,
		GL_SHORT,
		GL_UNSIGNED_SHORT,
		GL_INT,
		GL_UNSIGNED_INT,
		GL_FLOAT,
		GL_HALF_FLOAT,
		GL_DOUBLE,
		GL_INT_2_10_10_10_REV,
		GL_UNSIGNED_INT_2_10_10_10_REV,
		GL_UNSIGNED_INT_10F_11F_11F_REV};

	constexpr GLenum glFormatArray[][2]{
		{GL_NONE, GL_NONE},

		{GL_R8, GL_RED},
		{GL_SR8_EXT, GL_RED}, //need GL_EXT_texture_sRGB_R8	2015

		{GL_RG8, GL_RG},
		{GL_SRG8_EXT, GL_RG}, //need GL_EXT_texture_sRGB_RG8	2015

		{GL_RGB8, GL_RGB},
		{GL_SRGB8, GL_RGB},
		{GL_BGR_EXT, GL_BGR}, //need GL_EXT_bgra
		{GL_NONE, GL_NONE},	  //no matching for BGR_SRGB

		{GL_RGBA8, GL_RGBA},
		{GL_SRGB8_ALPHA8, GL_RGBA},
		{GL_BGRA8_EXT, GL_BGRA_EXT}, //need GL_EXT_texture_format_BGRA8888 2008
		{GL_NONE, GL_NONE},			 //no matching for BGRA_SRGB

		{GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT},
		{GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT},
		{GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT},
		{GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL},
		{GL_DEPTH32F_STENCIL8, GL_DEPTH_STENCIL},
		{GL_STENCIL_INDEX8, GL_STENCIL_INDEX},
	};
	constexpr GLenum glShaderStageArray[]{
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER,
		GL_GEOMETRY_SHADER,
		GL_TESS_CONTROL_SHADER,
		GL_TESS_EVALUATION_SHADER,
		GL_COMPUTE_SHADER,
		GL_NONE};
	constexpr GLenum glShaderStageFlagBitsArray[]{
		GL_VERTEX_SHADER_BIT,		   // 0x00000001
		GL_FRAGMENT_SHADER_BIT,		   // 0x00000002
		GL_GEOMETRY_SHADER_BIT,		   // 0x00000004
		GL_TESS_CONTROL_SHADER_BIT,	   // 0x00000008
		GL_TESS_EVALUATION_SHADER_BIT, // 0x00000010
		GL_COMPUTE_SHADER_BIT,		   // 0x00000020
		GL_ALL_SHADER_BITS,			   // 0xFFFFFFFF
	};
	constexpr GLenum glBufferBindTargetArray[]{
		GL_COPY_READ_BUFFER,
		GL_COPY_WRITE_BUFFER,
		GL_TEXTURE_BUFFER,
		0, //storage texel
		GL_UNIFORM_BUFFER,
		GL_SHADER_STORAGE_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER,
		GL_ARRAY_BUFFER,
		GL_DRAW_INDIRECT_BUFFER,
		GL_TRANSFORM_FEEDBACK_BUFFER,
	};
	constexpr GLbitfield glBufferMapFlagBitArray[]{
		GL_MAP_READ_BIT,			  // 0x0001
		GL_MAP_WRITE_BIT,			  // 0x0002
		GL_MAP_INVALIDATE_RANGE_BIT,  // 0x0004
		GL_MAP_INVALIDATE_BUFFER_BIT, // 0x0008
		GL_MAP_FLUSH_EXPLICIT_BIT,	  // 0x0010
		GL_MAP_UNSYNCHRONIZED_BIT,	  // 0x0020
		GL_MAP_PERSISTENT_BIT,		  // 0x0040
		GL_MAP_COHERENT_BIT,		  // 0x0080
	};
	constexpr GLbitfield glBufferStorageFlagBitArray[]{
		GL_MAP_READ_BIT,		// 0x0001
		GL_MAP_WRITE_BIT,		// 0x0002
		GL_MAP_PERSISTENT_BIT,	// 0x0040
		GL_MAP_COHERENT_BIT,	// 0x0080
		GL_DYNAMIC_STORAGE_BIT, // 0x0100
		GL_CLIENT_STORAGE_BIT,	// 0x0200
	};
	constexpr GLenum glMutableStorageUsageArray[]{
		GL_STREAM_DRAW,
		GL_STREAM_READ,
		GL_STREAM_COPY,
		GL_DYNAMIC_DRAW,
		GL_DYNAMIC_READ,
		GL_DYNAMIC_COPY,
		GL_STATIC_DRAW,
		GL_STATIC_READ,
		GL_STATIC_COPY,
	};
	constexpr GLenum glImageTypeArray[]{
		GL_TEXTURE_1D_ARRAY,
		GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_3D,
	};
	constexpr GLenum glImageViewTypeArray[]{
		GL_TEXTURE_1D,
		GL_TEXTURE_2D,
		GL_TEXTURE_3D,
		GL_TEXTURE_CUBE_MAP,
		GL_TEXTURE_1D_ARRAY,
		GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_CUBE_MAP_ARRAY,
	};
	constexpr GLenum glFilterArray[]{
		GL_NEAREST,
		GL_LINEAR,
	};

	constexpr GLenum glSamplerWrapModeArray[]{
		GL_REPEAT,
		GL_MIRRORED_REPEAT,
		GL_CLAMP_TO_EDGE,
		GL_CLAMP_TO_BORDER,
	};
	constexpr GLenum glCompareOpArray[]{
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS};
	constexpr GLenum glComponentSwizzleArray[]{
		GL_NONE,
		GL_ZERO,
		GL_ONE,
		GL_RED,
		GL_GREEN,
		GL_BLUE,
		GL_ALPHA};
	constexpr GLenum glPrimitiveTopologyArray[]{
		GL_POINTS,
		GL_LINES,
		GL_LINE_STRIP,
		GL_TRIANGLES,
		GL_TRIANGLE_STRIP,
		GL_TRIANGLE_FAN,
		GL_LINES_ADJACENCY,
		GL_LINE_STRIP_ADJACENCY,
		GL_TRIANGLES_ADJACENCY,
		GL_TRIANGLE_STRIP_ADJACENCY,
		GL_PATCHES};
	constexpr GLenum glPolygonModeArray[]{
		GL_FILL,
		GL_LINE,
		GL_POINT,
	};
	constexpr GLenum glCullModeArray[]{
		GL_NONE,
		GL_FRONT,
		GL_BACK,
		GL_FRONT_AND_BACK};

	//set clipcontrol origin
	constexpr GLenum glFrontFaceArray[]{
#ifdef CLIP_ORIGIN_UPPER_LEFT
		GL_CW,
		GL_CCW,
#else
		GL_CCW,
		GL_CW,
#endif
	};
	constexpr GLenum glStencilOpArray[]{
		GL_KEEP,
		GL_ZERO,
		GL_REPLACE,
		GL_INCR,
		GL_DECR,
		GL_INVERT,
		GL_INCR_WRAP,
		GL_DECR_WRAP};
	constexpr GLenum glLogicOpArray[]{
		GL_CLEAR,
		GL_AND,
		GL_AND_REVERSE,
		GL_COPY,
		GL_AND_INVERTED,
		GL_NOOP,
		GL_XOR,
		GL_OR,
		GL_NOR,
		GL_EQUIV,
		GL_INVERT,
		GL_OR_REVERSE,
		GL_COPY_INVERTED,
		GL_OR_INVERTED,
		GL_NAND,
		GL_SET};
	constexpr GLenum glBlendFactorArray[]{
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_CONSTANT_COLOR,
		GL_ONE_MINUS_CONSTANT_COLOR,
		GL_CONSTANT_ALPHA,
		GL_ONE_MINUS_CONSTANT_ALPHA,
		GL_SRC_ALPHA_SATURATE,
		GL_SRC1_COLOR,
		GL_ONE_MINUS_SRC1_COLOR,
		GL_SRC1_ALPHA,
		GL_ONE_MINUS_SRC1_ALPHA,
	};
	constexpr GLenum glBlendOpArray[]{
		GL_FUNC_ADD,
		GL_FUNC_SUBTRACT,
		GL_FUNC_REVERSE_SUBTRACT,
		GL_MIN,
		GL_MAX,
	};
	GLenum Map(BlendOp op)
	{
		return glBlendOpArray[static_cast<size_t>(op)];
	}
	GLenum Map(BlendFactor factor)
	{
		return glBlendFactorArray[static_cast<size_t>(factor)];
	}
	GLenum Map(LogicOp op)
	{
		return glLogicOpArray[static_cast<size_t>(op)];
	}
	GLenum Map(StencilOp op)
	{
		return glStencilOpArray[static_cast<size_t>(op)];
	}
	GLenum Map(FrontFace face)
	{
		return glFrontFaceArray[static_cast<size_t>(face)];
	}
	GLenum Map(CullMode mode)
	{
		return glCullModeArray[static_cast<size_t>(mode)];
	}
	GLenum Map(PolygonMode mode)
	{
		return glPolygonModeArray[static_cast<size_t>(mode)];
	}
	GLenum Map(PrimitiveTopology topology)
	{
		return glPrimitiveTopologyArray[static_cast<size_t>(topology)];
	}
	GLenum Map(ComponentSwizzle swizzle)
	{
		return glComponentSwizzleArray[static_cast<size_t>(swizzle)];
	}
	GLenum Map(ShaderStageFlagBits flag)
	{
		switch (flag)
		{
		case ShaderStageFlagBits::VERTEX_BIT:
			return GL_VERTEX_SHADER;
		case ShaderStageFlagBits::FRAGMENT_BIT:
			return GL_FRAGMENT_SHADER;
		case ShaderStageFlagBits::GEOMETRY_BIT:
			return GL_GEOMETRY_SHADER;
		case ShaderStageFlagBits::TESSELLATION_CONTROL_BIT:
			return GL_TESS_CONTROL_SHADER;
		case ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT:
			return GL_TESS_EVALUATION_SHADER;
		case ShaderStageFlagBits::COMPUTE_BIT:
			return GL_COMPUTE_SHADER;
		default:
			THROW("GL do not contain shader stage:" + std::to_string(int(flag)));
		}
	}
	GLbitfield MapShaderStageFlags(ShaderStageFlagBits flags)
	{
		if (static_cast<bool>(flags == ShaderStageFlagBits::ALL))
			return GL_ALL_SHADER_BITS;
		if (static_cast<bool>(flags == ShaderStageFlagBits::ALL_GRAPHICS))
			return 0x1F;
		GLbitfield ret{};
		if (static_cast<bool>(flags & ShaderStageFlagBits::VERTEX_BIT))
			ret |= GL_VERTEX_SHADER_BIT;
		if (static_cast<bool>(flags & ShaderStageFlagBits::FRAGMENT_BIT))
			ret |= GL_FRAGMENT_SHADER_BIT;
		if (static_cast<bool>(flags & ShaderStageFlagBits::TESSELLATION_CONTROL_BIT))
			ret |= GL_TESS_CONTROL_SHADER_BIT;
		if (static_cast<bool>(flags & ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT))
			ret |= GL_TESS_EVALUATION_SHADER_BIT;
		if (static_cast<bool>(flags & ShaderStageFlagBits::GEOMETRY_BIT))
			ret |= GL_GEOMETRY_SHADER_BIT;
		if (static_cast<bool>(flags & ShaderStageFlagBits::COMPUTE_BIT))
			ret |= GL_COMPUTE_SHADER_BIT;
		return ret;
	}
	GLenum MapInternalFormat(ShitFormat format)
	{
		return glFormatArray[static_cast<size_t>(format)][0];
	}
	GLenum MapExternalFormat(ShitFormat format)
	{
		return glFormatArray[static_cast<size_t>(format)][1];
	}
	GLenum MapDataTypeFromFormat(ShitFormat format)
	{
		switch (format)
		{
		case ShitFormat::D32_SFLOAT:
		case ShitFormat::D24_UNORM_S8_UINT:
		case ShitFormat::D32_SFLOAT_S8_UINT:
			return GL_FLOAT;
		default:
			return GL_UNSIGNED_BYTE;
		}
	}
	GLenum Map(BufferMutableStorageUsage usage)
	{
		return glMutableStorageUsageArray[static_cast<size_t>(usage)];
	}
	GLbitfield Map(BufferMapFlagBits flag)
	{
		return static_cast<GLbitfield>(flag);
	}
	GLbitfield Map(BufferStorageFlagBits flag)
	{
		return static_cast<GLbitfield>(flag);
	}
	GLenum Map(SamplerWrapMode wrapMode)
	{
		return glSamplerWrapModeArray[static_cast<size_t>(wrapMode)];
	}
	GLenum Map(CompareOp op)
	{
		return glCompareOpArray[static_cast<size_t>(op)];
	}
	GLenum Map(Filter filter)
	{
		return glFilterArray[static_cast<size_t>(filter)];
	}
	GLenum Map(ImageType imageType, SampleCountFlagBits sampleCountFlag)
	{
		if (sampleCountFlag > SampleCountFlagBits::BIT_1)
			return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
		else
			return glImageTypeArray[static_cast<size_t>(imageType)];
	}
	GLenum Map(ImageViewType viewType, SampleCountFlagBits sampleCountFlag)
	{
		if (sampleCountFlag > SampleCountFlagBits::BIT_1)
		{
			if (viewType == ImageViewType::TYPE_2D)
				return GL_TEXTURE_2D_MULTISAMPLE;
			else if (viewType == ImageViewType::TYPE_2D_ARRAY)
				return GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
			else
				THROW("wrong image view type");
		}
		else
			return glImageViewTypeArray[static_cast<size_t>(viewType)];
	}
	GLenum Map(DataType dataType)
	{
		return glDataTypeArray[static_cast<size_t>(dataType)];
	}
	GLenum Map(IndexType type)
	{
		switch (type)
		{
		case IndexType::UINT8:
			return GL_UNSIGNED_BYTE;
		case IndexType::UINT16:
			return GL_UNSIGNED_SHORT;
		case IndexType::UINT32:
			return GL_UNSIGNED_INT;
		default:
			THROW("wrong index type");
		}
	}
	std::variant<std::array<float, 4>, std::array<int32_t, 4>> Map(BorderColor color)
	{
		switch (color)
		{
		case BorderColor::FLOAT_OPAQUE_BLACK:
			return std::array<float, 4>{0.f, 0.f, 0.f, 1.f};
		case BorderColor::FLOAT_OPAQUE_WHITE:
			return std::array<float, 4>{1.f, 1.f, 1.f, 1.f};
		case BorderColor::FLOAT_TRANSPARENT_BLACK:
		default:
			return std::array<float, 4>{0.f, 0.f, 0.f, 0.f};
		case BorderColor::INT_OPAQUE_BLACK:
			return std::array<int32_t, 4>{0, 0, 0, 1};
		case BorderColor::INT_OPAQUE_WHITE:
			return std::array<int32_t, 4>{1, 1, 1, 1};
		case BorderColor::INT_TRANSPARENT_BLACK:
			return std::array<int32_t, 4>{0, 0, 0, 0};
		}
	}

} // namespace Shit
