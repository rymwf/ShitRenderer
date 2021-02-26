/**
 * @file ShitRendererPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <string>
#include <memory>
#include <unordered_set>
#include <vector>
#include <stdexcept>
#include <string>

#include "ShitEnum.h"
#include "config.h"

#ifdef SHIT_DLL
#define SHIT_API __declspec(dllexport)
#else
#define SHIT_API
#endif // SHIT_DLL

#define SHIT_DEFAULT_WINDOW_X 100
#define SHIT_DEFAULT_WINDOW_Y 60
#define SHIT_DEFAULT_WINDOW_WIDTH 800 
#define SHIT_DEFAULT_WINDOW_HEIGHT 600

#define SHIT_RENDERER_LOAD_FUNC "ShitLoadRenderSystem"
#define SHIT_RENDERER_DELETE_FUNC "ShitDeleteRenderSystem"
#define SHIT_RENDERER_GL_NAME "GLRenderer"
#define SHIT_RENDERER_D3D11_NAME "D3D11Renderer"
#define SHIT_RENDERER_D3D12_NAME "D3D12Renderer"
#define SHIT_RENDERER_METAL_NAME "MetalRenderer"
#define SHIT_RENDERER_VULKAN_NAME "VKRenderer"

#ifdef NDEBUG
#define LOG(str)
#define LOG_VAR(str)
#else
#include <iostream>
#define LOG(str) \
	std::cout << __FILE__ << " " << __LINE__ << ":  " << str << std::endl
#define LOG_VAR(str) \
	std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl
#endif

#define THROW(str) throw std::runtime_error(__FILE__ " " + std::to_string(__LINE__) + ": " + str);

namespace Shit
{
	class ShitWindow;

	using PhysicalDevice = void *;

	//common object types
	struct Offset2D
	{
		int32_t x;
		int32_t y;
	};
	struct Offset3D
	{
		int32_t x;
		int32_t y;
		int32_t z;
	};
	struct Extent2D
	{
		uint32_t width;
		uint32_t height;
	};
	struct Extent3D
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
	};
	struct Rect2D
	{
		Offset2D offset;
		Extent2D extent;
	};

	struct RenderSystemCreateInfo
	{
		RendererVersion version;
		RenderSystemCreateFlagBits flags;
	};

	struct WindowCreateInfo{
		const char *name;
		Rect2D rect;
	};

	struct ContextCreateInfo
	{
		ShitWindow *pWindow;
		PhysicalDevice phyicalDevice; //opengl this should be nullptr
	};

	//TODO: gl pixel format attributes
	struct GLPixelFormatAttributes
	{
		//WGL_NUMBER_PIXEL_FORMATS_ARB
		//WGL_DRAW_TO_WINDOW_ARB
		//WGL_DRAW_TO_BITMAP_ARB
		//WGL_ACCELERATION_ARB
		//WGL_NEED_PALETTE_ARB
		//WGL_NEED_SYSTEM_PALETTE_ARB
		//WGL_SWAP_LAYER_BUFFERS_ARB,
		//WGL_SWAP_METHOD_ARB
		//WGL_NUMBER_OVERLAYS_ARB
		//WGL_NUMBER_UNDERLAYS_ARB
		//WGL_TRANSPARENT_ARB
		//WGL_TRANSPARENT_RED_VALUE_ARB
		//WGL_TRANSPARENT_GREEN_VALUE_ARB
		//WGL_TRANSPARENT_BLUE_VALUE_ARB
		//WGL_TRANSPARENT_ALPHA_VALUE_ARB
		//WGL_TRANSPARENT_INDEX_VALUE_ARB
		//WGL_SHARE_DEPTH_ARB
		//WGL_SHARE_STENCIL_ARB
		//WGL_SHARE_ACCUM_ARB
		//WGL_SUPPORT_GDI_ARB
		//WGL_SUPPORT_OPENGL_ARB
		//WGL_DOUBLE_BUFFER_ARB
		//WGL_STEREO_ARB
		//WGL_PIXEL_TYPE_ARB
		//WGL_COLOR_BITS_ARB
		//WGL_RED_BITS_ARB
		//WGL_RED_SHIFT_ARB
		//WGL_GREEN_BITS_ARB
		//WGL_GREEN_SHIFT_ARB
		//WGL_BLUE_BITS_ARB
		//WGL_BLUE_SHIFT_ARB
		//WGL_ALPHA_BITS_ARB
		//WGL_ALPHA_SHIFT_ARB
		//WGL_ACCUM_BITS_ARB
		//WGL_ACCUM_RED_BITS_ARB
		//WGL_ACCUM_GREEN_BITS_ARB
		//WGL_ACCUM_BLUE_BITS_ARB
		//WGL_ACCUM_ALPHA_BITS_ARB
		//WGL_DEPTH_BITS_ARB
		//WGL_STENCIL_BITS_ARB
		//WGL_AUX_BUFFERS_ARB
	};

} // namespace Shit
