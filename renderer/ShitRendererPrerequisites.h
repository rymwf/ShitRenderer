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
#include <optional>
#include <utility>

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
	class Device;
	class Swapchain;

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

	struct WindowCreateInfo
	{
		const char *name;
		Rect2D rect;
	};

	struct SwapchainCreateInfo
	{
		Device *device;
		ShitWindow *pWindow;
		uint32_t minImageCount;
		ShitFormat format;		   //!<no use for opengl, opengl is always RGBA
		ColorSpace colorSpace;	   //!<sRGB
		Extent2D imageExtent;	   //no use for opengl
		uint32_t imageArrayLayers; //!< alway 1 unless you are developing a stereoscopic 3D applicaiton
		PresentMode presentMode;
	};

	struct ShaderModuleCreateInfo
	{
		ShaderStageFlagBits stage;
		std::string code;
	};

	struct BufferCreateInfo
	{
		BufferCreateFlagBits flags;
		uint32_t size;
		union
		{
			BufferStorageFlagBits storageFlags;
			BufferMutableStorageUsage storageUsage;
		};
	};

} // namespace Shit
