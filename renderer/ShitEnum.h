/**
 * @file ShitEnum.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <type_traits>
namespace Shit
{

	/**
	 * https://wiggling-bits.net/using-enum-classes-as-type-safe-bitmasks/
	 */
	template <typename Enum>
	struct EnableBitMaskOperators
	{
		static const bool enable = false;
	};

#define ENABLE_BITMASK_OPERATORS(x)      \
	template <>                          \
	struct EnableBitMaskOperators<x>     \
	{                                    \
		static const bool enable = true; \
	};

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator|(Enum lhs, Enum rhs)
	{
		return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) | static_cast<std::underlying_type_t<Enum>>(rhs));
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator&(Enum lhs, Enum rhs)
	{
		return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) & static_cast<std::underlying_type_t<Enum>>(rhs));
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator^(Enum lhs, Enum rhs)
	{
		return static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) ^ static_cast<std::underlying_type_t<Enum>>(rhs));
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator~(Enum rhs)
	{
		return static_cast<Enum>(~static_cast<std::underlying_type_t<Enum>>(rhs));
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	bool operator!(Enum rhs)
	{
		return !static_cast<bool>(rhs);
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum &operator|=(Enum &lhs, Enum rhs)
	{
		lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) | static_cast<std::underlying_type_t<Enum>>(rhs));
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum &operator&=(Enum &lhs, Enum rhs)
	{
		lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) & static_cast<std::underlying_type_t<Enum>>(rhs));
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum &operator^=(Enum &lhs, Enum rhs)
	{
		lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) ^ static_cast<std::underlying_type_t<Enum>>(rhs));
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator>>(Enum lhs, uint32_t offset)
	{
		lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) >> offset);
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum operator<<(Enum lhs, uint32_t offset)
	{
		lhs = static_cast<Enum>(static_cast<std::underlying_type_t<Enum>>(lhs) << offset);
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum &operator>>=(Enum &lhs, uint32_t offset)
	{
		lhs = lhs >> offset;
		return lhs;
	}

	template <typename Enum, typename std::enable_if_t<std::is_enum_v<Enum> && EnableBitMaskOperators<Enum>::enable, bool> = true>
	Enum &operator<<=(Enum &lhs, uint32_t offset)
	{
		lhs = lhs << offset;
		return lhs;
	}

	enum class EventType
	{
		NONE,
		MOUSEMOVE,
		MOUSEBUTTON,
		MOUSEWHEEL,
		KEYBOARD,
		DROP,
		CHAR,
		WINDOW_CREATE,
		WINDOW_CLOSE,
		WINDOW_DESTROY,
		//WINDOW_QUIT,
		WINDOW_RESIZE,
		WINDOW_CONTENTSACLE,
		WINDOW_POS,
		WINDOW_ICONIFY,
		WINDOW_MAXIMIZE,
		WINDOW_FOCUS,
		WINDOW_REFRESH,
		WINDOW_COUNT
	};
	enum class KeyCode
	{
		KEY_NONE,
		KEY_SPACE,
		KEY_APOSTROPHE,
		KEY_COMMA,
		KEY_MINUS,
		KEY_PERIOD,
		KEY_SLASH,
		KEY_0,
		KEY_1,
		KEY_2,
		KEY_3,
		KEY_4,
		KEY_5,
		KEY_6,
		KEY_7,
		KEY_8,
		KEY_9,
		KEY_SEMICOLON,
		KEY_EQUAL,
		KEY_A,
		KEY_B,
		KEY_C,
		KEY_D,
		KEY_E,
		KEY_F,
		KEY_G,
		KEY_H,
		KEY_I,
		KEY_J,
		KEY_K,
		KEY_L,
		KEY_M,
		KEY_N,
		KEY_O,
		KEY_P,
		KEY_Q,
		KEY_R,
		KEY_S,
		KEY_T,
		KEY_U,
		KEY_V,
		KEY_W,
		KEY_X,
		KEY_Y,
		KEY_Z,
		KEY_LEFT_BRACKET,
		KEY_BACKSLASH,
		KEY_RIGHT_BRACKET,
		KEY_GRAVE_ACCENT,
		//KEY_WORLD_1,
		//KEY_WORLD_2,

		//function key
		KEY_ESCAPE,
		KEY_ENTER,
		KEY_TAB,
		KEY_BACKSPACE,
		KEY_INSERT,
		KEY_DELETE,
		KEY_RIGHT,
		KEY_LEFT,
		KEY_DOWN,
		KEY_UP,
		KEY_PAGE_UP,
		KEY_PAGE_DOWN,
		KEY_HOME,
		KEY_END,
		KEY_CAPS_LOCK,
		KEY_SCROLL_LOCK,
		KEY_NUM_LOCK,
		KEY_PRINT_SCREEN,
		KEY_PAUSE,
		KEY_F1,
		KEY_F2,
		KEY_F3,
		KEY_F4,
		KEY_F5,
		KEY_F6,
		KEY_F7,
		KEY_F8,
		KEY_F9,
		KEY_F10,
		KEY_F11,
		KEY_F12,

		KEY_KP_0,
		KEY_KP_1,
		KEY_KP_2,
		KEY_KP_3,
		KEY_KP_4,
		KEY_KP_5,
		KEY_KP_6,
		KEY_KP_7,
		KEY_KP_8,
		KEY_KP_9,
		KEY_KP_DECIMAL,
		KEY_KP_DIVIDE,
		KEY_KP_MULTIPLY,
		KEY_KP_SUBTRACT,
		KEY_KP_ADD,
		//	KEY_KP_ENTER,
		//	KEY_KP_EQUAL,
		KEY_LEFT_SHIFT,
		KEY_LEFT_CONTROL,
		KEY_LEFT_ALT,
		//	KEY_LEFT_SUPER,
		KEY_RIGHT_SHIFT,
		KEY_RIGHT_CONTROL,
		KEY_RIGHT_ALT,
		//	KEY_RIGHT_SUPER,
		//	KEY_MENU,

		KEY_COUNT
	};

	enum class EventModifierBits
	{
		None = 0,
		ALT = 0x1,
		CTRL = 0x2,
		SHIFT = 0x4,
		META = 0x8,
		BUTTONL = 0x10, //mouse left button
		BUTTONR = 0x20,
		BUTTONM = 0x40,
		BUTTONX1 = 0x80,
		BUTTONX2 = 0x100
	};
	ENABLE_BITMASK_OPERATORS(EventModifierBits);

	enum class PressAction
	{
		NONE,
		DOWN,
		UP,
		REPEAT,
		UNKNOWN
	};

	enum class MouseButton
	{
		MOUSE_NONE,
		MOUSE_L,
		MOUSE_M,
		MOUSE_R,
	};

	enum class DataType
	{
		BYTE,
		UNSIGNED_BYTE,
		SHORT,
		UNSIGNED_SHORT,
		INT,
		UNSIGNED_INT,
		FLOAT,
		FLOAT_HALF,
		DOUBLE,
		INT_2_10_10_10_REV,
		UNSIGNED_INT_2_10_10_10_REV,
		UNSIGNED_INT_10F_11F_11F_REV
	};

	enum class ShadingLanguage
	{
		GLSL = (0x10000),			//!< GLSL (OpenGL Shading Language).
		GLSL_110 = (0x10000 | 110), //!< GLSL 1.10 (since OpenGL 2.0).
		GLSL_120 = (0x10000 | 120), //!< GLSL 1.20 (since OpenGL 2.1).
		GLSL_130 = (0x10000 | 130), //!< GLSL 1.30 (since OpenGL 3.0).
		GLSL_140 = (0x10000 | 140), //!< GLSL 1.40 (since OpenGL 3.1).
		GLSL_150 = (0x10000 | 150), //!< GLSL 1.50 (since OpenGL 3.2).
		GLSL_330 = (0x10000 | 330), //!< GLSL 3.30 (since OpenGL 3.3).
		GLSL_400 = (0x10000 | 400), //!< GLSL 4.00 (since OpenGL 4.0).
		GLSL_410 = (0x10000 | 410), //!< GLSL 4.10 (since OpenGL 4.1).
		GLSL_420 = (0x10000 | 420), //!< GLSL 4.20 (since OpenGL 4.2).
		GLSL_430 = (0x10000 | 430), //!< GLSL 4.30 (since OpenGL 4.3).
		GLSL_440 = (0x10000 | 440), //!< GLSL 4.40 (since OpenGL 4.4).
		GLSL_450 = (0x10000 | 450), //!< GLSL 4.50 (since OpenGL 4.5).
		GLSL_460 = (0x10000 | 460), //!< GLSL 4.60 (since OpenGL 4.6).

		ESSL = (0x20000),			//!< ESSL (OpenGL ES Shading Language).
		ESSL_100 = (0x20000 | 100), //!< ESSL 1.00 (since OpenGL ES 2.0).
		ESSL_300 = (0x20000 | 300), //!< ESSL 3.00 (since OpenGL ES 3.0).
		ESSL_310 = (0x20000 | 310), //!< ESSL 3.10 (since OpenGL ES 3.1).
		ESSL_320 = (0x20000 | 320), //!< ESSL 3.20 (since OpenGL ES 3.2).

		HLSL = (0x30000),			 //!< HLSL (High Level Shading Language).
		HLSL_2_0 = (0x30000 | 200),	 //!< HLSL 2.0 (since Direct3D 9).
		HLSL_2_0a = (0x30000 | 201), //!< HLSL 2.0a (since Direct3D 9a).
		HLSL_2_0b = (0x30000 | 202), //!< HLSL 2.0b (since Direct3D 9b).
		HLSL_3_0 = (0x30000 | 300),	 //!< HLSL 3.0 (since Direct3D 9c).
		HLSL_4_0 = (0x30000 | 400),	 //!< HLSL 4.0 (since Direct3D 10).
		HLSL_4_1 = (0x30000 | 410),	 //!< HLSL 4.1 (since Direct3D 10.1).
		HLSL_5_0 = (0x30000 | 500),	 //!< HLSL 5.0 (since Direct3D 11).
		HLSL_5_1 = (0x30000 | 510),	 //!< HLSL 5.1 (since Direct3D 11.3).
		HLSL_6_0 = (0x30000 | 600),	 //!< HLSL 6.0 (since Direct3D 12). Shader model 6.0 adds wave intrinsics and 64-bit integer types to HLSL.
		HLSL_6_1 = (0x30000 | 601),	 //!< HLSL 6.1 (since Direct3D 12). Shader model 6.1 adds \c SV_ViewID and \c SV_Barycentrics semantics to HLSL.
		HLSL_6_2 = (0x30000 | 602),	 //!< HLSL 6.2 (since Direct3D 12). Shader model 6.2 adds 16-bit scalar types to HLSL.
		HLSL_6_3 = (0x30000 | 603),	 //!< HLSL 6.3 (since Direct3D 12). Shader model 6.3 adds ray tracing (DXR) to HLSL.
		HLSL_6_4 = (0x30000 | 604),	 //!< HLSL 6.4 (since Direct3D 12). Shader model 6.4 adds machine learning intrinsics to HLSL.

		Metal = (0x40000),			 //!< Metal Shading Language.
		Metal_1_0 = (0x40000 | 100), //!< Metal 1.0 (since iOS 8.0).
		Metal_1_1 = (0x40000 | 110), //!< Metal 1.1 (since iOS 9.0 and OS X 10.11).
		Metal_1_2 = (0x40000 | 120), //!< Metal 1.2 (since iOS 10.0 and macOS 10.12).
		Metal_2_0 = (0x40000 | 200), //!< Metal 2.0 (since iOS 11.0 and macOS 10.13).
		Metal_2_1 = (0x40000 | 210), //!< Metal 2.1 (since iOS 12.0 and macOS 10.14).

		SPIRV = (0x50000),			 //!< SPIR-V Shading Language.
		SPIRV_100 = (0x50000 | 100), //!< SPIR-V 1.0.
		SPIRV_110 = (0x50000 | 110), //!< SPIR-V 1.1.
		SPIRV_120 = (0x50000 | 120), //!< SPIR-V 1.2.
		SPIRV_130 = (0x50000 | 130), //!< SPIR-V 1.3.
		SPIRV_140 = (0x50000 | 140), //!< SPIR-V 1.4. May 7, 2019
		SPIRV_150 = (0x50000 | 150), //!< SPIR-V 1.5. Sep 13, 2019

		VersionBitmask = 0x0000ffff, //!< Bitmask for the version number of each shading language enumeration entry.
	};

	enum class ShaderBinaryFormat
	{
		SPIR_V,
	};

	enum class ShaderStageFlagBits
	{
		VERTEX_BIT = 0x00000001,
		FRAGMENT_BIT = 0x00000002,
		GEOMETRY_BIT = 0x00000004,
		TESS_CONTROL_BIT = 0x00000008,
		TESS_EVALUATION_BIT = 0x00000010,
		COMPUTE_BIT = 0x00000020,
		ALL_BITS = 0x7FFFFFFF,
	};
	ENABLE_BITMASK_OPERATORS(ShaderStageFlagBits);

	enum class RendererVersion
	{
		GL = (0x10000),			  //!< OpenGL lastest.
		GL_110 = (0x10000 | 110), //!< OpenGL 1.1.	1997
		GL_120 = (0x10000 | 120), //!< OpenGL 1.2.	1998
		GL_121 = (0x10000 | 121), //!< OpenGL 1.2.1	1998
		GL_130 = (0x10000 | 130), //!< OpenGL 1.3.	2001
		GL_140 = (0x10000 | 140), //!< OpenGL 1.4.	2002
		GL_150 = (0x10000 | 150), //!< OpenGL 1.5.	2003
		GL_200 = (0x10000 | 200), //!< OpenGL 2.0.	2004
		GL_210 = (0x10000 | 210), //!< OpenGL 2.1.	2006
		GL_300 = (0x10000 | 300), //!< OpenGL 3.0.	2008
		GL_310 = (0x10000 | 310), //!< OpenGL 3.1.	2009
		GL_320 = (0x10000 | 320), //!< OpenGL 3.2.	2009
		GL_330 = (0x10000 | 330), //!< OpenGL 3.3.	2010
		GL_400 = (0x10000 | 400), //!< OpenGL 4.0.	2010
		GL_410 = (0x10000 | 410), //!< OpenGL 4.1.	2010
		GL_420 = (0x10000 | 420), //!< OpenGL 4.2.	2011
		GL_430 = (0x10000 | 430), //!< OpenGL 4.3.	2012
		GL_440 = (0x10000 | 440), //!< OpenGL 4.4.	2013
		GL_450 = (0x10000 | 450), //!< OpenGL 4.5.	2014
		GL_460 = (0x10000 | 460), //!< OpenGL 4.6.	2017

		GLES = (0x20000),			//!< OpenGL ES lastest
		GLES_100 = (0x20000 | 100), //!< OpenGL ES 1.0.
		GLES_120 = (0x20000 | 200), //!< OpenGL ES 2.0.
		GLES_300 = (0x20000 | 300), //!< OpenGL ES 3.0.
		GLES_310 = (0x20000 | 310), //!< OpenGL ES 3.1.
		GLES_320 = (0x20000 | 320), //!< OpenGL ES 3.2.

		VULKAN = (0x30000),			  //!< Vulkan lastest
		VULKAN_100 = (0x30000 | 100), //!< Vulkan 1.0
		VULKAN_110 = (0x30000 | 110), //!< Vulkan 1.1
		VULKAN_120 = (0x30000 | 120), //!< Vulkan 1.2

		METAL = (0x40000),
		D3D11 = (0x50000),
		D3D12 = (0x60000),

		VersionBitmask = 0x0000ffff, //!< Bitmask for the version number of each shading language enumeration entry.
		TypeBitmask = 0xf0000,		 //!< Typemask for renderer check
	};
	ENABLE_BITMASK_OPERATORS(RendererVersion);

	enum class RenderSystemCreateFlagBits
	{
		None = 0,
		SHIT_CONTEXT_DEBUG_BIT = (0x1),

		SHIT_GL_CONTEXT_FORWARD_COMPATIBLE_BIT = (0x2),
		SHIT_GL_CONTEXT_CORE_PROFILE_BIT = (0x4), //!< default for opengl
		SHIT_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT = (0x8),
	};
	ENABLE_BITMASK_OPERATORS(RenderSystemCreateFlagBits);

	enum class ColorSpace
	{
		SRGB_NONLINEAR,
	};

	enum class ShitFormat
	{
		UNDEFINED, //!< not supported in opengl

		R8_UNORM,
		R8_SRGB,

		RG8_UNORM,
		RG8_SRGB,

		RGB8_UNORM,
		RGB8_SRGB,
		BGR8_UNORM,
		BGR8_SRGB,

		RGBA8_UNORM,
		RGBA8_SRGB,
		BGRA8_UNORM,
		BGRA8_SRGB,

		D16_UNORM,
		D24_UNORM,
		D32_SFLOAT,
		D24_UNORM_S8_UINT,
		D32_SFLOAT_S8_UINT,
		S8_UINT,
	};

	enum class PresentMode
	{
		IMMEDIATE,
		FIFO, //vertical synchronous
	};

	enum class DescriptorType
	{
		SAMPLER,				//sampler (vulkan)
		COMBINED_IMAGE_SAMPLER, //sampler2D
		SAMPLED_IMAGE,			//texture2D (vulkan)
		STORAGE_IMAGE,			//image2D
		UNIFORM_TEXEL_BUFFER,	//samplerbuffer	(access to buffer texture,can only be accessed with texelFetch function) ,textureBuffer(vulkan)
		STORAGE_TEXEL_BUFFER,	//imagebuffer (access to buffer texture)
		UNIFORM_BUFFER,			//uniform block
		STORAGE_BUFFER,			//buffer block
		UNIFORM_BUFFER_DYNAMIC,
		STORAGE_BUFFER_DYNAMIC,
		INPUT_ATTACHMENT0,
	};

	enum class BufferUsageFlagBits
	{
		TRANSFER_SRC_BIT = 0x1,
		TRANSFER_DST_BIT = 0x2,
		UNIFORM_TEXEL_BUFFER_BIT = 0x4,
		STORAGE_TEXEL_BUFFER_BIT = 0x8,
		UNIFORM_BUFFER_BIT = 0x10,
		STORAGE_BUFFER_BIT = 0x20,
		INDEX_BUFFER_BIT = 0x40,
		VERTEX_BUFFER_BIT = 0x80,
		INDIRECT_BUFFER_BIT = 0x100,
		TRANSFORM_FEEDBACK_BUFFER_BIT = 0x200,
	};
	ENABLE_BITMASK_OPERATORS(BufferUsageFlagBits);

	enum class BufferStorageFlagBits
	{
		MAP_READ_BIT = 0x1,
		MAP_WRITE_BIT = 0x2,
		MAP_PERSISTENT_BIT = 0x40,
		MAP_COHERENT_BIT = 0x80,
		DYNAMIC_STORAGE_BIT = 0x100,
		CLIENT_STORAGE_BIT = 0x200,
	};
	ENABLE_BITMASK_OPERATORS(BufferStorageFlagBits);

	enum class BufferMutableStorageUsage
	{
		STREAM_DRAW,
		STREAM_READ,
		STREAM_COPY,
		DYNAMIC_DRAW,
		DYNAMIC_READ,
		DYNAMIC_COPY,
		STATIC_DRAW,
		STATIC_READ,
		STATIC_COPY,
	};
	enum class BufferMapFlagBits
	{
		READ_BIT = 0x1,
		WRITE_BIT = 0x2,
		INVALIDATE_RANGE_BIT = 0x4,
		INVALIDATE_BUFFER_BIT = 0x8,
		FLUSH_EXPLICIT_BIT = 0x10,
		UNSYNCHRONIZED_BIT = 0x20,
		PERSISTENT_BIT = 0x40,
		COHERENT_BIT = 0x80,
	};
	ENABLE_BITMASK_OPERATORS(BufferMapFlagBits);

	enum class MemoryPropertyFlagBits
	{
		DEVICE_LOCAL_BIT = 0x1,
		HOST_VISIBLE_BIT = 0x2,
		HOST_COHERENT_BIT = 0x4,
	};
	ENABLE_BITMASK_OPERATORS(MemoryPropertyFlagBits);

	enum class BufferCreateFlagBits
	{
		MUTABLE_FORMAT_BIT = 0x1, //for opengl
	};
	ENABLE_BITMASK_OPERATORS(BufferCreateFlagBits);

	enum class ImageCreateFlagBits
	{
		MUTABLE_FORMAT_BIT = 0x1, //!< cannot use image view
		RENDER_BUFFER_BIT = 0x2
	};
	ENABLE_BITMASK_OPERATORS(ImageCreateFlagBits);

	enum class ImageType
	{
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
	};

	enum class ImageViewType
	{
		TYPE_1D,
		TYPE_2D,
		TYPE_3D,
		TYPE_CUBE,
		TYPE_1D_ARRAY,
		TYPE_2D_ARRAY,
		TYPE_CUBE_ARRAY,
	};

	enum class SampleCountFlagBits
	{
		BIT_1 = 0x00000001,
		BIT_2 = 0x00000002,
		BIT_4 = 0x00000004,
		BIT_8 = 0x00000008,
		BIT_16 = 0x00000010,
		BIT_32 = 0x00000020,
		BIT_64 = 0x00000040,
	};

	enum class Filter
	{
		NEAREST,
		LINEAR,
	};

	enum SamplerMipmapMode
	{
		NEAREST,
		LINEAR,
	};

	enum class SamplerWrapMode
	{
		REPEAT,
		MIRRORED_REPEAT,
		CLAMP_TO_EDGE,
		CLAMP_TO_BORDER,
	};
	enum class CompareOp
	{
		NEVER,
		LESS,
		EQUAL,
		LESS_OR_EQUAL,
		GREATER,
		NOT_EQUAL,
		GREATER_OR_EQUAL,
		ALWAYS,
	};

	enum class ComponentSwizzle
	{
		IDENTITY,
		ZERO,
		ONE,
		R,
		G,
		B,
		A,
	};

	enum class AttachmentLoadOp
	{
		LOAD,
		CLEAR, //glClear
		DONT_CARE,
	};

	enum class AttachmentStoreOp
	{
		STORE, //where to store
		DONT_CARE,
	};

	enum class PipelineBindPoint
	{
		GRAPHICS,
		COMPUTE,
	};

	enum class CommandPoolCreateFlagBits
	{
	};
	ENABLE_BITMASK_OPERATORS(CommandPoolCreateFlagBits);

	enum class CommandBufferLevel
	{
		PRIMARY,
		SECONDARY
	};
	enum class QueueFlagBits
	{
		GRAPHICS_BIT = 0x1,
		COMPUTE_BIT = 0x2,
		TRANSFER_BIT = 0x4,
		SPARSE_BINDING_BIT = 0x8,
		PROTECTED_BIT = 0x10,
	};
	ENABLE_BITMASK_OPERATORS(QueueFlagBits);

	enum class Result
	{
		SUCCESS,
		NOT_READY,
		TIME_OUT,
		EVENT_SET,
		EVENT_RESET,
		INCOMPLETE,
	};
}