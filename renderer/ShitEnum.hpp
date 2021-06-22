/**
 * @file ShitEnum.hpp
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
	enum class WindowCreateFlagBits
	{
		INVISIBLE = 0x1,
		FIXED_SIZE = 0x2,
	};
	ENABLE_BITMASK_OPERATORS(WindowCreateFlagBits);

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
		KEY_LBUTTON, //        0x01
		KEY_RBUTTON, //        0x02
		KEY_CANCEL,	 //         0x03
		KEY_MBUTTON, //        0x04    /* NOT contiguous with L & RBUTTON */

		KEY_BACK, //           0x08
		KEY_TAB,  //            0x09

		KEY_CLEAR,	//          0x0C
		KEY_RETURN, //         0x0D

		KEY_SHIFT,	 //          0x10
		KEY_CONTROL, //        0x11
		KEY_MENU,	 //           0x12
		KEY_PAUSE,	 //          0x13
		KEY_CAPITAL, //        0x14

		KEY_KANA,	 //           0x15
		KEY_HANGEUL, //        0x15  /* old name - should be here for compatibility */
		KEY_HANGUL,	 //         0x15
		KEY_IME_ON,	 //         0x16
		KEY_JUNJA,	 //          0x17
		KEY_FINAL,	 //          0x18
		KEY_HANJA,	 //          0x19
		KEY_KANJI,	 //          0x19
		KEY_IME_OFF, //        0x1A

		KEY_ESCAPE, //         0x1B

		KEY_CONVERT,	//        0x1C
		KEY_NONCONVERT, //     0x1D
		KEY_ACCEPT,		//         0x1E
		KEY_MODECHANGE, //     0x1F

		KEY_SPACE,	  //          0x20
		KEY_PRIOR,	  //          0x21
		KEY_NEXT,	  //           0x22
		KEY_END,	  //            0x23
		KEY_HOME,	  //           0x24
		KEY_LEFT,	  //           0x25
		KEY_UP,		  //             0x26
		KEY_RIGHT,	  //          0x27
		KEY_DOWN,	  //           0x28
		KEY_SELECT,	  //         0x29
		KEY_PRINT,	  //          0x2A
		KEY_EXECUTE,  //        0x2B
		KEY_SNAPSHOT, //       0x2C
		KEY_INSERT,	  //         0x2D
		KEY_DELETE,	  //         0x2E
		KEY_HELP,	  //           0x2F

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

		KEY_LWIN, //           0x5B
		KEY_RWIN, //           0x5C
		KEY_APPS, //           0x5D

		/*
 * 0x5E : reserved
 */

		KEY_SLEEP, //          0x5F

		KEY_NUMPAD0,   //        0x60
		KEY_NUMPAD1,   //        0x61
		KEY_NUMPAD2,   //        0x62
		KEY_NUMPAD3,   //        0x63
		KEY_NUMPAD4,   //        0x64
		KEY_NUMPAD5,   //        0x65
		KEY_NUMPAD6,   //        0x66
		KEY_NUMPAD7,   //        0x67
		KEY_NUMPAD8,   //        0x68
		KEY_NUMPAD9,   //        0x69
		KEY_MULTIPLY,  //       0x6A
		KEY_ADD,	   //            0x6B
		KEY_SEPARATOR, //      0x6C
		KEY_SUBTRACT,  //       0x6D
		KEY_DECIMAL,   //        0x6E
		KEY_DIVIDE,	   //         0x6F
		KEY_F1,		   //             0x70
		KEY_F2,		   //             0x71
		KEY_F3,		   //             0x72
		KEY_F4,		   //             0x73
		KEY_F5,		   //             0x74
		KEY_F6,		   //             0x75
		KEY_F7,		   //             0x76
		KEY_F8,		   //             0x77
		KEY_F9,		   //             0x78
		KEY_F10,	   //            0x79
		KEY_F11,	   //            0x7A
		KEY_F12,	   //            0x7B
		KEY_F13,	   //            0x7C
		KEY_F14,	   //            0x7D
		KEY_F15,	   //            0x7E
		KEY_F16,	   //            0x7F
		KEY_F17,	   //            0x80
		KEY_F18,	   //            0x81
		KEY_F19,	   //            0x82
		KEY_F20,	   //            0x83
		KEY_F21,	   //            0x84
		KEY_F22,	   //            0x85
		KEY_F23,	   //            0x86
		KEY_F24,	   //            0x87

		KEY_NUMLOCK, //        0x90
		KEY_SCROLL,	 //         0x91

		/*
 * NEC PC-9800 kbd definitions
 */
		KEY_EQUAL, //KEY_OEM_NEC_EQUAL, //  0x92   // '=' key on numpad

		/*
 * Fujitsu/OASYS kbd definitions
 */
		KEY_OEM_FJ_JISHO,	//   0x92   // 'Dictionary' key
		KEY_OEM_FJ_MASSHOU, // 0x93   // 'Unregister word' key
		KEY_OEM_FJ_TOUROKU, // 0x94   // 'Register word' key
		KEY_OEM_FJ_LOYA,	//    0x95   // 'Left OYAYUBI' key
		KEY_OEM_FJ_ROYA,	//    0x96   // 'Right OYAYUBI' key

		/*
 * 0x97 - 0x9F : unassigned
 */

		/*
 * KEY_L* & VK_R* - left and right Alt, Ctrl and Shift virtual keys.
 * Used only as parameters to GetAsyncKeyState() and GetKeyState().
 * No other API or message will distinguish left and right keys in this way.
 */
		KEY_LSHIFT,	  //         0xA0
		KEY_RSHIFT,	  //         0xA1
		KEY_LCONTROL, //       0xA2
		KEY_RCONTROL, //       0xA3
		KEY_LMENU,	  //          0xA4
		KEY_RMENU,	  //          0xA5

		/*
 * 0xB8 - 0xB9 : reserved
 */

		KEY_SEMICOLON, //KEY_OEM_1,	   //          0xBA   // ';:' for US
		KEY_PLUS,	   //KEY_OEM_PLUS,   //       0xBB   // '+' any country
		KEY_COMMA,	   //KEY_OEM_COMMA,  //      0xBC   // ',' any country
		KEY_MINUS,	   //KEY_OEM_MINUS,  //      0xBD   // '-' any country
		KEY_PERIOD,	   //KEY_OEM_PERIOD, //     0xBE   // '.' any country
		KEY_SLASH,	   //KEY_OEM_2,	   //          0xBF   // '/?' for US
		KEY_TILDE,	   //KEY_OEM_3,	   //          0xC0   // '`~' for US

		/*
 * 0xC1 - 0xC2 : reserved
 */

		KEY_LBRACKET,	//KEY_OEM_4, //          0xDB  //  '[{' for US
		KEY_BACK_SLASH, //KEY_OEM_5, //          0xDC  //  '\|' for US
		KEY_RBRACKET,	//KEY_OEM_6, //          0xDD  //  ']}' for US
		KEY_QUOTE,		//KEY_OEM_7, //          0xDE  //  ''"' for US
		KEY_OEM_8,		//          0xDF

		/*
 * 0xE0 : reserved
 */

		/*
 * Various extended or enhanced keyboards
 */
		KEY_OEM_AX,	  //         0xE1  //  'AX' key on Japanese AX kbd
		KEY_OEM_102,  //        0xE2  //  "<>" or "\|" on RT 102-key kbd.
		KEY_ICO_HELP, //       0xE3  //  Help key on ICO
		KEY_ICO_00,	  //         0xE4  //  00 key on ICO

		KEY_ICO_CLEAR, //      0xE6

		/*
 * 0xE8 : unassigned
 */

		/*
 * Nokia/Ericsson definitions
 */
		//KEY_OEM_RESET,	//      0xE9
		//KEY_OEM_JUMP,	//       0xEA
		//KEY_OEM_PA1,		//        0xEB
		//KEY_OEM_PA2,		//        0xEC
		//KEY_OEM_PA3,		//        0xED
		//KEY_OEM_WSCTRL,	//     0xEE
		//KEY_OEM_CUSEL,	//      0xEF
		//KEY_OEM_ATTN,	//       0xF0
		//KEY_OEM_FINISH,	//     0xF1
		//KEY_OEM_COPY,	//       0xF2
		//KEY_OEM_AUTO,	//       0xF3
		//KEY_OEM_ENLW,	//       0xF4
		//KEY_OEM_BACKTAB, //    0xF5

		//KEY_ATTN,	  //           0xF6
		//KEY_CRSEL,	  //          0xF7
		//KEY_EXSEL,	  //          0xF8
		//KEY_EREOF,	  //          0xF9
		//KEY_PLAY,	  //           0xFA
		//KEY_ZOOM,	  //           0xFB
		//KEY_NONAME,	  //         0xFC
		//KEY_PA1,		  //            0xFD
		//KEY_OEM_CLEAR, //      0xFE

		KEY_COUNT
	};

	enum class EventModifierBits
	{
		None = 0,
		ALTL = 0x1,
		CTRLL = 0x2,
		SHIFTL = 0x4,
		META = 0x8,
		BUTTONL = 0x10, //mouse left button
		BUTTONR = 0x20,
		BUTTONM = 0x40,
		BUTTONX1 = 0x80,
		BUTTONX2 = 0x100,
		ALTR = 0x200,
		CTRLR = 0x400,
		SHIFTR = 0x800,
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
		//INT_2_10_10_10_REV,
		//UNSIGNED_INT_2_10_10_10_REV,
		//UNSIGNED_INT_10F_11F_11F_REV
		UNSIGNED_INT_24_8,
	};
	static uint32_t dataTypeSizeArray[]{
		1, //BYTE,
		1, //UNSIGNED_BYTE,
		2, //SHORT,
		2, //UNSIGNED_SHORT,
		4, //INT,
		4, //UNSIGNED_INT,
		4, //FLOAT,
		2, //FLOAT_HALF,
		8, //DOUBLE,
		//1,////INT_2_10_10_10_REV,
		//1,////UNSIGNED_INT_2_10_10_10_REV,
		//1,////UNSIGNED_INT_10F_11F_11F_REV
		5, //UNSIGNED_INT_24_8,
	};

	inline uint32_t GetDataTypeSize(DataType type)
	{
		return dataTypeSizeArray[static_cast<size_t>(type)];
	}

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

	//same as vulkan
	enum class ShaderStageFlagBits
	{
		VERTEX_BIT = 0x00000001,
		TESSELLATION_CONTROL_BIT = 0x00000002,
		TESSELLATION_EVALUATION_BIT = 0x00000004,
		GEOMETRY_BIT = 0x00000008,
		FRAGMENT_BIT = 0x00000010,
		COMPUTE_BIT = 0x00000020,
		ALL_GRAPHICS = 0x0000001F,
		ALL = 0x7FFFFFFF,
		RAYGEN_BIT = 0x00000100,
		ANY_HIT_BIT = 0x00000200,
		CLOSEST_HIT_BIT = 0x00000400,
		MISS_BIT = 0x00000800,
		INTERSECTION_BIT = 0x00001000,
		CALLABLE_BIT = 0x00002000,
		TASK_BIT = 0x00000040,
		MESH_BIT = 0x00000080,
	};
	ENABLE_BITMASK_OPERATORS(ShaderStageFlagBits);

	enum class RendererVersion
	{
		GL = (0x10000),				//!< OpenGL lastest.
		GL_110 = (0x10000 | 0x110), //!< OpenGL 1.1.	1997
		GL_120 = (0x10000 | 0x120), //!< OpenGL 1.2.	1998
		GL_121 = (0x10000 | 0x121), //!< OpenGL 1.2.1	1998
		GL_130 = (0x10000 | 0x130), //!< OpenGL 1.3.	2001
		GL_140 = (0x10000 | 0x140), //!< OpenGL 1.4.	2002
		GL_150 = (0x10000 | 0x150), //!< OpenGL 1.5.	2003
		GL_200 = (0x10000 | 0x200), //!< OpenGL 2.0.	2004
		GL_210 = (0x10000 | 0x210), //!< OpenGL 2.1.	2006
		GL_300 = (0x10000 | 0x300), //!< OpenGL 3.0.	2008
		GL_310 = (0x10000 | 0x310), //!< OpenGL 3.1.	2009
		GL_320 = (0x10000 | 0x320), //!< OpenGL 3.2.	2009
		GL_330 = (0x10000 | 0x330), //!< OpenGL 3.3.	2010
		GL_400 = (0x10000 | 0x400), //!< OpenGL 4.0.	2010
		GL_410 = (0x10000 | 0x410), //!< OpenGL 4.1.	2010
		GL_420 = (0x10000 | 0x420), //!< OpenGL 4.2.	2011
		GL_430 = (0x10000 | 0x430), //!< OpenGL 4.3.	2012
		GL_440 = (0x10000 | 0x440), //!< OpenGL 4.4.	2013
		GL_450 = (0x10000 | 0x450), //!< OpenGL 4.5.	2014
		GL_460 = (0x10000 | 0x460), //!< OpenGL 4.6.	2017

		GLES = (0x20000),			  //!< OpenGL ES lastest
		GLES_100 = (0x20000 | 0x100), //!< OpenGL ES 1.0.
		GLES_120 = (0x20000 | 0x200), //!< OpenGL ES 2.0.
		GLES_300 = (0x20000 | 0x300), //!< OpenGL ES 3.0.
		GLES_310 = (0x20000 | 0x310), //!< OpenGL ES 3.1.
		GLES_320 = (0x20000 | 0x320), //!< OpenGL ES 3.2.

		VULKAN = (0x30000),				//!< Vulkan lastest
		VULKAN_100 = (0x30000 | 0x100), //!< Vulkan 1.0
		VULKAN_110 = (0x30000 | 0x110), //!< Vulkan 1.1
		VULKAN_120 = (0x30000 | 0x120), //!< Vulkan 1.2

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
		Num
	};

	enum class ShitFormat
	{
		UNDEFINED, //!< not supported in opengl

		R8_UNORM,
		R8_SNORM,
		R8_SRGB,
		R8_UI,
		R8_SI,

		RG8_UNORM,
		RG8_SNORM,
		RG8_SRGB,
		RG8_UI,
		RG8_SI,

		RGB8_UNORM,
		RGB8_SNORM,
		RGB8_SRGB,
		RGB8_UI,
		RGB8_SI,
		BGR8_UNORM,
		BGR8_SRGB,
		BGR8_UI,
		BGR8_SI,

		RGBA8_UNORM,
		RGBA8_SNORM,
		RGBA8_SRGB,
		RGBA8_UI,
		RGBA8_SI,
		BGRA8_UNORM,
		BGRA8_SRGB,
		BGRA8_UI,
		BGRA8_SI,

		R16_UNORM,
		R16_SNORM,
		R16_UI,
		R16_SI,
		R16_SFLOAT,

		RG16_UNORM,
		RG16_SNORM,
		RG16_UI,
		RG16_SI,
		RG16_SFLOAT,

		RGB16_UNORM,
		RGB16_SNORM,
		RGB16_UI,
		RGB16_SI,
		RGB16_SFLOAT,

		RGBA16_UNORM,
		RGBA16_SNORM,
		RGBA16_UI,
		RGBA16_SI,
		RGBA16_SFLOAT,

		R32_UI,
		RG32_UI,
		RGB32_UI,
		RGBA32_UI,

		R32_SI,
		RG32_SI,
		RGB32_SI,
		RGBA32_SI,

		R32_SFLOAT,
		RG32_SFLOAT,
		RGB32_SFLOAT,
		RGBA32_SFLOAT,

		D16_UNORM,
		D24_UNORM,
		D32_SFLOAT,
		D24_UNORM_S8_UINT,
		D32_SFLOAT_S8_UINT,
		S8_UINT,
		Num
	};

	struct ShitFormatAttribute
	{
		DataType dataType;
		uint32_t componentNum;
		bool normalized;
	};

	//component num, component size, normalized
	static ShitFormatAttribute ShitFormatAttributeArray[]{
		{}, //UNDEFINED, //!< not supported in opengl

		{DataType::UNSIGNED_BYTE, 1, true},	 //R8_UNORM,
		{DataType::BYTE, 1, true},			 //R8_SNORM,
		{DataType::UNSIGNED_BYTE, 1, true},	 //R8_SRGB,
		{DataType::UNSIGNED_BYTE, 1, false}, //R8_UI,
		{DataType::BYTE, 1, false},			 //R8_SI,

		{DataType::UNSIGNED_BYTE, 2, true},	 //RG8_UNORM,
		{DataType::BYTE, 2, true},			 //RG8_SNORM,
		{DataType::UNSIGNED_BYTE, 2, true},	 //RG8_SRGB,
		{DataType::UNSIGNED_BYTE, 2, false}, //RG8_UI,
		{DataType::BYTE, 2, false},			 //RG8_SI,

		{DataType::UNSIGNED_BYTE, 3, true},	 //RGB8_UNORM,
		{DataType::BYTE, 3, true},			 //RGB8_SNORM,
		{DataType::UNSIGNED_BYTE, 3, true},	 //RGB8_SRGB,
		{DataType::UNSIGNED_BYTE, 3, false}, //RGR8_UI,
		{DataType::BYTE, 3, false},			 //RGR8_SI,
		{DataType::UNSIGNED_BYTE, 3, true},	 //BGR8_UNORM,
		{DataType::UNSIGNED_BYTE, 3, true},	 //BGR8_SRGB,
		{DataType::UNSIGNED_BYTE, 3, false}, //BGR8_UI,
		{DataType::BYTE, 3, false},			 //BGR8_SI,

		{DataType::UNSIGNED_BYTE, 4, true},	 //RGBA8_UNORM,
		{DataType::BYTE, 4, true},			 //RGBA8_SNORM,
		{DataType::UNSIGNED_BYTE, 4, true},	 //RGBA8_SRGB,
		{DataType::UNSIGNED_BYTE, 4, false}, //RGBA8_UI,
		{DataType::BYTE, 4, false},			 //RGBA8_SI,
		{DataType::UNSIGNED_BYTE, 4, true},	 //BGRA8_UNORM,
		{DataType::BYTE, 4, true},			 //BGRA8_SRGB,
		{DataType::UNSIGNED_BYTE, 4, false}, //BGRA8_UI,
		{DataType::BYTE, 4, false},			 //BGRA8_SI,

		{DataType::UNSIGNED_SHORT, 1, true},  //R16_UNORM,
		{DataType::SHORT, 1, true},			  //R16_SNORM,
		{DataType::UNSIGNED_SHORT, 1, false}, //R16_UI,
		{DataType::SHORT, 1, false},		  //R16_SI,
		{DataType::FLOAT_HALF, 1, true},	  //R16_SFLOAT,

		{DataType::UNSIGNED_SHORT, 2, true},  //RG16_UNORM,
		{DataType::SHORT, 2, true},			  //RG16_SNORM,
		{DataType::UNSIGNED_SHORT, 2, false}, //RG16_UI,
		{DataType::SHORT, 2, false},		  //RG16_SI,
		{DataType::FLOAT_HALF, 2, true},	  //RG16_SFLOAT,

		{DataType::UNSIGNED_SHORT, 3, true},  //RGB16_UNORM,
		{DataType::SHORT, 3, true},			  //RGB16_SNORM,
		{DataType::UNSIGNED_SHORT, 3, false}, //RGB16_UI,
		{DataType::SHORT, 3, false},		  //RGB16_SI,
		{DataType::FLOAT_HALF, 3, true},	  //RGB16_SFLOAT,

		{DataType::UNSIGNED_SHORT, 4, true},  //RGBA16_UNORM,
		{DataType::SHORT, 4, true},			  //RGBA16_SNORM,
		{DataType::UNSIGNED_SHORT, 4, false}, //RGBA16_UI,
		{DataType::SHORT, 4, false},		  //RGBA16_SI,
		{DataType::FLOAT_HALF, 4, true},	  //RGBA16_SFLOAT,

		{DataType::UNSIGNED_INT, 1, false}, //R32_UI,
		{DataType::UNSIGNED_INT, 2, false}, //RG32_UI,
		{DataType::UNSIGNED_INT, 3, false}, //RGB32_UI,
		{DataType::UNSIGNED_INT, 4, false}, //RGBA32_UI,

		{DataType::INT, 1, false}, //R32_SI,
		{DataType::INT, 2, false}, //RG32_SI,
		{DataType::INT, 3, false}, //RGB32_SI,
		{DataType::INT, 4, false}, //RGBA32_SI,

		{DataType::FLOAT, 1, false}, //R32_SFLOAT,
		{DataType::FLOAT, 2, false}, //RG32_SFLOAT,
		{DataType::FLOAT, 3, false}, //RGB32_SFLOAT,
		{DataType::FLOAT, 4, false}, //RGBA32_SFLOAT,

		{DataType::UNSIGNED_SHORT, 1, true},	 //D16_UNORM,
		{DataType::BYTE, 3, true},				 //D24_UNORM,
		{DataType::FLOAT, 1, false},			 //D32_SFLOAT,
		{DataType::UNSIGNED_INT_24_8, 1, false}, //D24_UNORM_S8_UINT,
		{DataType::BYTE, 5, false},				 //D32_SFLOAT_S8_UINT,	//??????
		{DataType::UNSIGNED_BYTE, 1, false},	 //S8_UINT,
	};

	inline DataType GetFormatDataType(ShitFormat format)
	{
		return ShitFormatAttributeArray[static_cast<size_t>(format)].dataType;
	}
	inline uint32_t GetFormatComponentNum(ShitFormat format)
	{
		return ShitFormatAttributeArray[static_cast<size_t>(format)].componentNum;
	}
	inline bool GetFormatNormalized(ShitFormat format)
	{
		return ShitFormatAttributeArray[static_cast<size_t>(format)].normalized;
	}

	enum class PresentMode
	{
		IMMEDIATE,
		MAILBOX,
		FIFO,
		FIFO_RELAXED,
		SHARED_DEMAND_REFRESH,
		SHARED_CONTINUOUS_REFRESH,
		Num
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
		Num,
		None = 0xFFFF,
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
		HOST_VISIBLE_BIT = 0x2, //
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
		SPARSE_BINDING_BIT = 0x00000001,
		SPARSE_RESIDENCY_BIT = 0x00000002,
		SPARSE_ALIASED_BIT = 0x00000004,
		MUTABLE_FORMAT_BIT = 0x00000008, //mean the format of imageview canbe different from the original, different from GL_IMMUTABLE_FORMAT_BIT
		CUBE_COMPATIBLE_BIT = 0x00000010,
		ALIAS_BIT = 0x00000400,
		SPLIT_INSTANCE_BIND_REGIONS_BIT = 0x00000040,
		ARRAY_2D_COMPATIBLE_BIT = 0x00000020,
		BLOCK_TEXEL_VIEW_COMPATIBLE_BIT = 0x00000080,
		EXTENDED_USAGE_BIT = 0x00000100,
		PROTECTED_BIT = 0x00000800,
		DISJOINT_BIT = 0x00000200,
		CORNER_SAMPLED_BIT_NV = 0x00002000,
		SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT = 0x00001000,
		SUBSAMPLED_BIT_EXT = 0x00004000,
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
	enum class ImageTiling
	{
		OPTIMAL,
		LINEAR,
	};

	//same as vulkan
	enum class ImageLayout
	{
		UNDEFINED,
		GENERAL,
		COLOR_ATTACHMENT_OPTIMAL,
		DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		DEPTH_STENCIL_READ_ONLY_OPTIMAL,
		SHADER_READ_ONLY_OPTIMAL,
		TRANSFER_SRC_OPTIMAL,
		TRANSFER_DST_OPTIMAL,
		PREINITIALIZED,
		DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
		DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
		DEPTH_ATTACHMENT_OPTIMAL,
		DEPTH_READ_ONLY_OPTIMAL,
		STENCIL_ATTACHMENT_OPTIMAL,
		STENCIL_READ_ONLY_OPTIMAL,
		PRESENT_SRC,
		SHARED_PRESENT,
		SHADING_RATE_OPTIMAL,
		FRAGMENT_DENSITY_MAP_OPTIMAL,
	};

	enum class ImageUsageFlagBits
	{
		TRANSFER_SRC_BIT = 0x1,
		TRANSFER_DST_BIT = 0x2,
		SAMPLED_BIT = 0x4,
		STORAGE_BIT = 0x8,
		COLOR_ATTACHMENT_BIT = 0x10,
		DEPTH_STENCIL_ATTACHMENT_BIT = 0x20,
		TRANSIENT_ATTACHMENT_BIT = 0x40, //rendererbuffer for opengl
		INPUT_ATTACHMENT_BIT = 0x80
	};
	ENABLE_BITMASK_OPERATORS(ImageUsageFlagBits);

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
		TRANSIENT_BIT = 0x1,
		RESET_COMMAND_BUFFER_BIT = 0x2,
	};
	ENABLE_BITMASK_OPERATORS(CommandPoolCreateFlagBits);

	enum class CommandBufferLevel
	{
		PRIMARY,
		SECONDARY
	};
	enum class CommandBufferUsageFlagBits
	{
		ONE_TIME_SUBMIT_BIT = 0x1,
		//RENDER_PASS_CONTINUE_BIT = 0x2,
		//SIMULTANEOUS_USE_BIT = 0x4,
	};
	ENABLE_BITMASK_OPERATORS(CommandBufferUsageFlagBits);

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
		TIMEOUT,
		EVENT_SET,
		EVENT_RESET,
		INCOMPLETE,
		SHIT_ERROR,
		SHIT_ERROR_OUT_OF_DATE
	};
	enum class SubpassContents
	{
		INLINE,
		SECONDARY_COMMAND_BUFFERS
	};
	enum class IndexType
	{
		NONE,
		UINT8,
		UINT16,
		UINT32,
	};

	inline uint32_t GetIndexTypeSize(IndexType type)
	{
		switch (type)
		{
		case IndexType::UINT8:
			return 1;
		case IndexType::UINT16:
		default:
			return 2;
		case IndexType::UINT32:
			return 4;
		}
	}

	enum class CommandBufferResetFlatBits
	{
		RELEASE_RESOURCES_BIT = 0x1,
	};
	ENABLE_BITMASK_OPERATORS(CommandBufferResetFlatBits);

	enum class PrimitiveTopology
	{
		POINT_LIST,
		LINE_LIST,
		LINE_STRIP,
		TRIANGLE_LIST,
		TRIANGLE_STRIP,
		TRIANGLE_FAN,
		LINE_LIST_WITH_ADJACENCY,
		LINE_STRIP_WITH_ADJACENCY,
		TRIANGLE_LIST_WITH_ADJACENCY,
		TRIANGLE_STRIP_WITH_ADJACENCY,
		PATCH_LIST
	};
	enum class PolygonMode
	{
		FILL,
		LINE,
		POINT,
	};
	enum class CullMode
	{
		NONE,
		FRONT,
		BACK,
		FRONT_AND_BACK,
	};
	enum class FrontFace
	{
		COUNTER_CLOCKWISE,
		CLOCKWISE,
	};
	enum class StencilOp
	{
		KEEP,
		ZERO,
		REPLACE,
		INCREMENT_AND_CLAMP,
		DECREMENT_AND_CLAMP,
		INVERT,
		INCREMENT_AND_WRAP,
		DECREMENT_AND_WRAP,
	};
	enum class LogicOp
	{
		CLEAR,
		AND,
		AND_REVERSE,
		COPY,
		AND_INVERTED,
		NO_OP,
		XOR,
		OR,
		NOR,
		EQUIVALENT,
		INVERT,
		OR_REVERSE,
		COPY_INVERTED,
		OR_INVERTED,
		NAND,
		SET,
	};
	enum class BlendFactor
	{
		ZERO,
		ONE,
		SRC_COLOR,
		ONE_MINUS_SRC_COLOR,
		DST_COLOR,
		ONE_MINUS_DST_COLOR,
		SRC_ALPHA,
		ONE_MINUS_SRC_ALPHA,
		DST_ALPHA,
		ONE_MINUS_DST_ALPHA,
		CONSTANT_COLOR,
		ONE_MINUS_CONSTANT_COLOR,
		CONSTANT_ALPHA,
		ONE_MINUS_CONSTANT_ALPHA,
		SRC_ALPHA_SATURATE,
		SRC1_COLOR,
		ONE_MINUS_SRC1_COLOR,
		SRC1_ALPHA,
		ONE_MINUS_SRC1_ALPHA,
	};
	enum class BlendOp
	{
		ADD,
		SUBTRACT,
		REVERSE_SUBTRACT,
		MIN,
		MAX,
	};
	enum class ColorComponentFlagBits
	{
		R_BIT = 0x1,
		G_BIT = 0x2,
		B_BIT = 0x4,
		A_BIT = 0x8,
	};
	ENABLE_BITMASK_OPERATORS(ColorComponentFlagBits);
	enum class DynamicState
	{
		VIEWPORT,
		SCISSOR,
		LINE_WIDTH,
		DEPTH_BIAS,
		BLEND_CONSTANTS,
		DEPTH_BOUNDS,
		STENCIL_COMPARE_MASK,
		STENCIL_WRITE_MASK,
		STENCIL_REFERENCE,
		VIEWPORT_W_SCALING,	// Provided by VK_NV_clip_space_w_scaling
		DISCARD_RECTANGLE,	// Provided by VK_EXT_discard_rectangles
		SAMPLE_LOCATIONS,// Provided by VK_EXT_sample_locations
		VIEWPORT_SHADING_RATE_PALETTE,// Provided by VK_NV_shading_rate_image
		VIEWPORT_COARSE_SAMPLE_ORDER,// Provided by VK_NV_shading_rate_image
		EXCLUSIVE_SCISSOR,// Provided by VK_NV_scissor_exclusive
		LINE_STIPPLE,// Provided by VK_EXT_line_rasterization
		// Provided by VK_EXT_extended_dynamic_state
		CULL_MODE,
		FRONT_FACE,
		PRIMITIVE_TOPOLOGY,
		VIEWPORT_WITH_COUNT,
		SCISSOR_WITH_COUNT,
		VERTEX_INPUT_BINDING_STRIDE,
		DEPTH_TEST_ENABLE,
		DEPTH_WRITE_ENABLE,
		DEPTH_COMPARE_OP,
		DEPTH_BOUNDS_TEST_ENABLE,
		STENCIL_TEST_ENABLE,
		STENCIL_OP,
		Num,
	};

	enum class FenceCreateFlagBits
	{
		SIGNALED_BIT = 0x1
	};
	ENABLE_BITMASK_OPERATORS(FenceCreateFlagBits);
	enum class SemaphoreType
	{
		BINARY,
		TIMELINE,
	};
	enum class BorderColor
	{
		FLOAT_TRANSPARENT_BLACK,
		INT_TRANSPARENT_BLACK,
		FLOAT_OPAQUE_BLACK,
		INT_OPAQUE_BLACK,
		FLOAT_OPAQUE_WHITE,
		INT_OPAQUE_WHITE,
		//		FLOAT_CUSTOM_EXT,
		//		INT_CUSTOM_EXT,
	};
	//same as vulkan
	enum class PipelineStageFlagBits
	{
		TOP_OF_PIPE_BIT = 0x00000001,
		DRAW_INDIRECT_BIT = 0x00000002,
		VERTEX_INPUT_BIT = 0x00000004,
		VERTEX_SHADER_BIT = 0x00000008,
		TESSELLATION_CONTROL_SHADER_BIT = 0x00000010,
		TESSELLATION_EVALUATION_SHADER_BIT = 0x00000020,
		GEOMETRY_SHADER_BIT = 0x00000040,
		FRAGMENT_SHADER_BIT = 0x00000080,
		EARLY_FRAGMENT_TESTS_BIT = 0x00000100,
		LATE_FRAGMENT_TESTS_BIT = 0x00000200,
		COLOR_ATTACHMENT_OUTPUT_BIT = 0x00000400,
		COMPUTE_SHADER_BIT = 0x00000800,
		TRANSFER_BIT = 0x00001000,
		BOTTOM_OF_PIPE_BIT = 0x00002000,
		HOST_BIT = 0x00004000,
		ALL_GRAPHICS_BIT = 0x00008000,
		ALL_COMMANDS_BIT = 0x00010000,
		TRANSFORM_FEEDBACK_BIT_EXT = 0x01000000,
		CONDITIONAL_RENDERING_BIT_EXT = 0x00040000,
		RAY_TRACING_SHADER_BIT_KHR = 0x00200000,
		ACCELERATION_STRUCTURE_BUILD_BIT_KHR = 0x02000000,
		SHADING_RATE_IMAGE_BIT_NV = 0x00400000,
		TASK_SHADER_BIT_NV = 0x00080000,
		MESH_SHADER_BIT_NV = 0x00100000,
		FRAGMENT_DENSITY_PROCESS_BIT_EXT = 0x00800000,
		COMMAND_PREPROCESS_BIT_NV = 0x00020000,
	};
	ENABLE_BITMASK_OPERATORS(PipelineStageFlagBits);
	enum class DependencyFlagBits
	{
		BY_REGION_BIT = 0x00000001,
		VIEW_LOCAL_BIT = 0x00000002,
		DEVICE_GROUP_BIT = 0x00000004,
	};
	ENABLE_BITMASK_OPERATORS(DependencyFlagBits);
	enum class AccessFlagBits
	{
		INDIRECT_COMMAND_READ_BIT = 0x00000001,
		INDEX_READ_BIT = 0x00000002,
		VERTEX_ATTRIBUTE_READ_BIT = 0x00000004,
		UNIFORM_READ_BIT = 0x00000008,
		INPUT_ATTACHMENT_READ_BIT = 0x00000010,
		SHADER_READ_BIT = 0x00000020,
		SHADER_WRITE_BIT = 0x00000040,
		COLOR_ATTACHMENT_READ_BIT = 0x00000080,
		COLOR_ATTACHMENT_WRITE_BIT = 0x00000100,
		DEPTH_STENCIL_ATTACHMENT_READ_BIT = 0x00000200,
		DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = 0x00000400,
		TRANSFER_READ_BIT = 0x00000800,
		TRANSFER_WRITE_BIT = 0x00001000,
		HOST_READ_BIT = 0x00002000,
		HOST_WRITE_BIT = 0x00004000,
		MEMORY_READ_BIT = 0x00008000,
		MEMORY_WRITE_BIT = 0x00010000,
		TRANSFORM_FEEDBACK_WRITE_BIT = 0x02000000,
		TRANSFORM_FEEDBACK_COUNTER_READ_BIT = 0x04000000,
		TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT = 0x08000000,
		CONDITIONAL_RENDERING_READ_BIT = 0x00100000,
		COLOR_ATTACHMENT_READ_NONCOHERENT_BIT = 0x00080000,
		ACCELERATION_STRUCTURE_READ_BIT = 0x00200000,
		ACCELERATION_STRUCTURE_WRITE_BIT = 0x00400000,
		SHADING_RATE_IMAGE_READ_BIT = 0x00800000,
		FRAGMENT_DENSITY_MAP_READ_BIT = 0x01000000,
		COMMAND_PREPROCESS_READ_BIT = 0x00020000,
		COMMAND_PREPROCESS_WRITE_BIT = 0x00040000,
	};
	ENABLE_BITMASK_OPERATORS(AccessFlagBits);
}