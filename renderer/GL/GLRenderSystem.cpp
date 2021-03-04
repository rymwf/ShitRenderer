/**
 * @file GLRenderSystem.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLRenderSystem.h"
#include <gl/wglext.h>
namespace Shit
{

	extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(const Shit::RenderSystemCreateInfo &createInfo)
	{
		return new GLRenderSystem(createInfo);
	}
	extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem)
	{
		delete pRenderSystem;
	}

	void GLRenderSystem::EnumeratePhysicalDevice([[maybe_unused]] std::vector<PhysicalDevice> &physicalDevices)
	{
		LOG("currently GL do not support select gpus");
	}

	Swapchain *GLRenderSystem::CreateSwapchain(const SwapchainCreateInfo &createInfo)
	{
#ifdef _WIN32
		mWindowAttributes.emplace_back(WindowAttribute{createInfo.pWindow, std::make_unique<GLSwapchainWin32>(createInfo, mCreateInfo.version, mCreateInfo.flags)});
#else
		static_assert(0, "CreateSwapchain is not implemented ye");
#endif
		return mWindowAttributes.back().pSwapchain.get();
	}
	void GLRenderSystem::ProcessWindowEvent([[maybe_unused]] const Event &ev)
	{
	}
	Shader *GLRenderSystem::CreateShader(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<GLShader>(createInfo));
		return mShaders.back().get();
	}
	void GLRenderSystem::DestroyShader(Shader *pShader)
	{
		for (auto it = mShaders.begin(), end = mShaders.end(); it != end; ++it)
		{
			if (it->get() == pShader)
			{
				mShaders.erase(it);
				break;
			}
		}
	}

	GraphicsPipeline *GLRenderSystem::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
	{
		mGraphicsPipelines.emplace_back(std::make_unique<GLGraphicsPipeline>(createInfo));
		return mGraphicsPipelines.back().get();
	}
	CommandBuffer *GLRenderSystem::CreateCommandBuffer(const CommandBufferCreateInfo &createInfo)
	{
		mCommandBuffers.emplace_back(std::make_unique<GLCommandBuffer>(createInfo));
		return mCommandBuffers.back().get();
	}

	Queue *GLRenderSystem::CreateDeviceQueue(const QueueCreateInfo &createInfo)
	{
		return nullptr;
	}
	Result GLRenderSystem::WaitForFence(Device *pDevice, Fence *fence, uint64_t timeout)
	{
		return Result::SUCCESS;
	}
} // namespace Shit
