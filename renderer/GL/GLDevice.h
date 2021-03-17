/**
 * @file GLDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include <renderer/ShitDevice.h>
#include "GLPrerequisites.h"
#include "GLSwapchain.h"
#include "GLShader.h"
#include "GLPipeline.h"
#include "GLCommandBuffer.h"
#include "GLBuffer.h"
#include "GLImage.h"
#include "GLDescriptor.h"
#include "GLSampler.h"

namespace Shit
{
	class GLDevice : public Device
	{
	protected:
		GLStateManager mStateManager{};

		RenderSystemCreateInfo mRenderSystemCreatInfo;

	public:
		GLDevice(const RenderSystemCreateInfo &createInfo)
			: mRenderSystemCreatInfo(createInfo) {}

		constexpr GLStateManager *GetStateManager()
		{
			return &mStateManager;
		}
		virtual void MakeCurrent() const = 0;

		CommandPool *CreateCommandPool(const CommandPoolCreateInfo &createInfo) override;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;

		Pipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) override;

		Image *CreateImage(const ImageCreateInfo &createInfo, void *pData) override;

		DescriptorSetLayout *CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) override;

		ImageView *CreateImageView(const ImageViewCreateInfo &createInfo) override;
		PipelineLayout *CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo) override;
		RenderPass *CreateRenderPass(const RenderPassCreateInfo &createInfo) override;
		Framebuffer *CreateFramebuffer(const FramebufferCreateInfo &createInfo) override;
		Semaphore *CreateDeviceSemaphore(const SemaphoreCreateInfo &createInfo) override;
		Fence *CreateFence(const FenceCreateInfo &createInfo) override;
	};

#ifdef _WIN32
	class GLDeviceWin32 final : public GLDevice
	{
		HDC mHDC;
		HGLRC mHRenderContext; //false context

	public:
		GLDeviceWin32(ShitWindow *pWindow, const RenderSystemCreateInfo &createInfo);
		~GLDeviceWin32() override
		{
			wglDeleteContext(mHRenderContext);
		}
		void MakeCurrent() const override
		{
			wglMakeCurrent(mHDC, mHRenderContext);
		}
		constexpr HDC GetHDC() const
		{
			return mHDC;
		}

		void GetWindowPixelFormats(const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats) override;

		Swapchain* CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;
	};
#endif

}