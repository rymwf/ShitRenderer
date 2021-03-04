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
#include <stack>
#include <utility>
#include <array>
#include <unordered_map>
#include <functional>
#include <type_traits>

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
	class Shader;
	struct Event;
	class CommandPool;
	class Semaphore;
	class CommandBuffer;
	class Fence;

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

	struct DeviceCreateInfo
	{
		union
		{
			PhysicalDevice *pPhysicalDevice; //no use for opengl
			ShitWindow *pWindow;			 //no use for vulkan
		};
	};

	struct WindowCreateInfo
	{
		const char *name;
		Rect2D rect;
		std::function<void(const Event &)> eventHandle;
	};

	struct SwapchainCreateInfo
	{
		Device *pDevice;	 
		ShitWindow *pWindow; //!<one window can only have one swapchain
		uint32_t minImageCount;
		ShitFormat format;		   //!<no use for opengl, opengl is always RGBA
		ColorSpace colorSpace;	   //!<sRGB
		Extent2D imageExtent;	   //!<no use for opengl
		uint32_t imageArrayLayers; //!< alway 1 unless you are developing a stereoscopic 3D applicaiton
		PresentMode presentMode;
	};

	struct ShaderCreateInfo
	{
		Device *pDevice;
		ShaderStageFlagBits stage;
		std::string code;
	};

	struct SpecializationInfo
	{
		std::vector<uint32_t> constantIDs;
		std::vector<uint32_t> constantValues;
	};

	struct PipelineShaderStageCreateInfo
	{
		ShaderStageFlagBits stage;
		Shader *pShader;
		const char* entryName;
		std::shared_ptr<SpecializationInfo> pSpecializationInfo;
	};

	struct VertexAttributeDescription
	{
		uint32_t location;	 //location in shader
		uint32_t binding;	 //index in given buffers
		uint32_t components; //1,2,3,4
		DataType dataType;
		bool normalized;
		uint32_t offset;
	};

	struct VertexBindingDescription
	{
		uint32_t binding; //index in given buffers
		uint32_t stride;
		uint32_t divisor; //attributes advance once per divior instances,when 0, advance per vertex
	};

	struct VertexInputStateCreateInfo
	{
		std::vector<VertexBindingDescription> vertexBindingDescriptions;
		std::vector<VertexAttributeDescription> vertexAttributeDescriptions;
	};
	struct GraphicsPipelineCreateInfo
	{
		Device *pDevice;
		std::shared_ptr<std::vector<PipelineShaderStageCreateInfo>> pStages;
		std::shared_ptr<VertexInputStateCreateInfo> pVertexInputState;
		//PipelineLayout layout;
	};

	struct BufferCreateInfo
	{
		Device *pDevice;
		BufferCreateFlagBits flags;
		uint32_t size;
		union
		{
			BufferStorageFlagBits storageFlags;
			BufferMutableStorageUsage storageUsage;
		};
	};

	struct CommandPoolCreateInfo
	{
		Device *pDevice;
		CommandPoolCreateFlagBits flags;
		uint32_t queueFamilyIndex;
	};
	struct CommandBufferCreateInfo
	{
		Device *pDevice;
		CommandPool *pCommandPool;
		CommandBufferLevel level;
	};

	struct QueueCreateInfo
	{
		Device *pDevice;
		QueueFlagBits queueFlags;
		uint32_t queueIndex;
		std::vector<uint32_t> skipQueueFamilyIndices;
	};

	struct SubmitInfo
	{
		std::vector<Semaphore *> waitSempahores; //wait pipeline stage is color attachment output
		std::vector<CommandBuffer *> commandBuffers;
		std::vector<Semaphore *> signalSempahores;
	};

	struct FenceCreateInfo
	{
		Device* pDevice;
	};

	struct SemaphoreCreateInfo
	{
		Device* pDevice;
	};

} // namespace Shit
