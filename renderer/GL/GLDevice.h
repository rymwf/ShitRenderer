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

		//std::shared_ptr<Swapchain> CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) = 0;

	public:
		GLDevice(const RenderSystemCreateInfo &createInfo)
			: mRenderSystemCreatInfo(createInfo) {}

		constexpr GLStateManager *GetStateManager()
		{
			return &mStateManager;
		}
		virtual void MakeCurrent() const = 0;

		Shader *CreateShader(const ShaderCreateInfo &createInfo) override;

		Pipeline *CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo) override;

		Queue *CreateDeviceQueue(const QueueCreateInfo &createInfo) override;

		Result WaitForFence(Fence *fence, uint64_t timeout) override;

		Buffer *CreateBuffer(const BufferCreateInfo &createInfo, void *pData) override;

		Image *CreateImage(const ImageCreateInfo &createInfo, void *pData) override;

		DescriptorSetLayout *CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo) override;
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

		Swapchain* CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;
	};
#endif

}