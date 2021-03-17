/**
 * @file GLDevice.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLDevice.h"
#include "GLFence.h"
#include "GLQueue.h"
#include "GLFramebuffer.h"
#include "GLSemaphore.h"
#include "GLPipeline.h"
#include "GLRenderPass.h"
#include "GLCommandPool.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{
	CommandPool *GLDevice::CreateCommandPool(const CommandPoolCreateInfo &createInfo)
	{
		mCommandPools.emplace_back(std::make_unique<GLCommandPool>(&mStateManager, createInfo));
		return mCommandPools.back().get();
	}

	Shader *GLDevice::CreateShader(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<GLShader>(&mStateManager, createInfo));
		return mShaders.back().get();
	}

	Pipeline *GLDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
	{
		mPipelines.emplace_back(std::make_unique<GLGraphicsPipeline>(&mStateManager, createInfo));
		return mPipelines.back().get();
	}

	Queue *GLDevice::CreateDeviceQueue(const QueueCreateInfo &createInfo)
	{
		mQueues.emplace_back(std::make_unique<GLQueue>(this, &mStateManager, createInfo));
		return mQueues.back().get();
	}
	Buffer *GLDevice::CreateBuffer(const BufferCreateInfo &createInfo, void *pData)
	{
		mBuffers.emplace_back(std::make_unique<GLBuffer>(&mStateManager, createInfo, pData));
		return mBuffers.back().get();
	}

	Image *GLDevice::CreateImage(const ImageCreateInfo &createInfo, void *pData)
	{
		mImages.emplace_back(std::make_unique<GLImage>(&mStateManager, createInfo));
		auto pImage = mImages.back().get();
		if (pData)
		{
			ImageSubData subdata{
				createInfo.format,
				DataType::UNSIGNED_BYTE,
				0,
				{{},
				 createInfo.extent},
				pData};
			static_cast<GLImage *>(pImage)->UpdateImageSubData(subdata);
		}
		return pImage;
	}
	DescriptorSetLayout *GLDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo &createInfo)
	{
		mDescriptorSetLayouts.emplace_back(std::make_unique<GLDescriptorSetLayout>(createInfo));
		return mDescriptorSetLayouts.back().get();
	}

	ImageView *GLDevice::CreateImageView(const ImageViewCreateInfo &createInfo)
	{
		mImageViews.emplace_back(std::make_unique<GLImageView>(&mStateManager, createInfo));
		return mImageViews.back().get();
	}
	PipelineLayout *GLDevice::CreatePipelineLayout(const PipelineLayoutCreateInfo &createInfo)
	{
		mPipelineLayouts.emplace_back(std::make_unique<GLPipelineLayout>(createInfo));
		return mPipelineLayouts.back().get();
	}
	RenderPass *GLDevice::CreateRenderPass(const RenderPassCreateInfo &createInfo)
	{
		mRenderPasses.emplace_back(std::make_unique<GLRenderPass>(createInfo));
		return mRenderPasses.back().get();
	}
	Framebuffer *GLDevice::CreateFramebuffer(const FramebufferCreateInfo &createInfo)
	{
		mFramebuffers.emplace_back(std::make_unique<GLFramebuffer>(&mStateManager, createInfo));
		return mFramebuffers.back().get();
	}
	Semaphore *GLDevice::CreateDeviceSemaphore(const SemaphoreCreateInfo &createInfo)
	{
		mSemaphores.emplace_back(std::make_unique<GLSemaphore>(&mStateManager, createInfo));
		return mSemaphores.back().get();
	}
	Fence *GLDevice::CreateFence(const FenceCreateInfo &createInfo)
	{
		mFences.emplace_back(std::make_unique<GLFence>(&mStateManager, createInfo));
		return mFences.back().get();
	}

#ifdef _WIN32

	void GLDeviceWin32::GetWindowPixelFormats([[maybe_unused]] const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats)
	{
		if (wglIsExtensionSupported("WGL_EXT_framebuffer_sRGB"))
		{
			formats.emplace_back(
				WindowPixelFormat{ShitFormat::RGBA8_SRGB,
								  ColorSpace::SRGB_NONLINEAR});
		}
		else if (wglIsExtensionSupported("WGL_ARB_framebuffer_sRGB"))
		{
			formats.emplace_back(
				WindowPixelFormat{ShitFormat::RGBA8_SRGB,
								  ColorSpace::SRGB_NONLINEAR});
		}
		formats.emplace_back(
			WindowPixelFormat{ShitFormat::RGBA8_UNORM,
							  ColorSpace::SRGB_NONLINEAR});
	}
	Swapchain *GLDeviceWin32::CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		mSwapchains.emplace_back(std::make_unique<GLSwapchainWin32>(this, &mStateManager, mHDC, createInfo, mRenderSystemCreatInfo.version, mRenderSystemCreatInfo.flags));
		pWindow->SetSwapchain(mSwapchains.back().get());
		return mSwapchains.back().get();
	}
	GLDeviceWin32::GLDeviceWin32(ShitWindow *pWindow, const RenderSystemCreateInfo &createInfo) : GLDevice(createInfo)
	{
		mHDC = GetDC(static_cast<WindowWin32 *>(pWindow)->GetHWND());
		if (!mHDC)
			THROW("failed to create window context");

		PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
			1,							   // version number
			PFD_DRAW_TO_WINDOW |		   // support window
				PFD_SUPPORT_OPENGL |	   // support OpenGL
				PFD_DOUBLEBUFFER,		   // double buffered
			PFD_TYPE_RGBA,				   // RGBA type
			32,							   // 32-bit color depth framebuffer bits
			0, 0, 0, 0, 0, 0,			   // color bits ignored
			0,							   // no alpha buffer
			0,							   // shift bit ignored
			0,							   // no accumulation buffer
			0, 0, 0, 0,					   // accum bits ignored
			24,							   // 24-bit z-buffer
			8,							   // 8-bit stencil buffer
			0,							   // no auxiliary buffer
			PFD_MAIN_PLANE,				   // main layer
			0,							   // reserved
			0, 0, 0						   // layer masks ignored
		};
		// get the best available match of pixel format for the device context
		int iPixelFormat = ChoosePixelFormat(mHDC, &pfd);
		LOG_VAR(iPixelFormat);
		SetPixelFormat(mHDC, iPixelFormat, &pfd);
		mHRenderContext = wglCreateContext(mHDC);
		if (!mHRenderContext)
			THROW("failed to create surface");

		wglMakeCurrent(mHDC, mHRenderContext);

		int majorversion, minorversion;
		glGetIntegerv(GL_MAJOR_VERSION, &majorversion);
		glGetIntegerv(GL_MINOR_VERSION, &minorversion);

		SHIT_GL_110 = majorversion * 10 + minorversion >= 11;
		SHIT_GL_120 = majorversion * 10 + minorversion >= 12;
		SHIT_GL_121 = majorversion * 10 + minorversion >= 13;
		SHIT_GL_130 = majorversion * 10 + minorversion >= 13;
		SHIT_GL_140 = majorversion * 10 + minorversion >= 14;
		SHIT_GL_150 = majorversion * 10 + minorversion >= 15;
		SHIT_GL_200 = majorversion * 10 + minorversion >= 20;
		SHIT_GL_210 = majorversion * 10 + minorversion >= 21;
		SHIT_GL_300 = majorversion * 10 + minorversion >= 30;
		SHIT_GL_310 = majorversion * 10 + minorversion >= 31;
		SHIT_GL_320 = majorversion * 10 + minorversion >= 32;
		SHIT_GL_330 = majorversion * 10 + minorversion >= 33;
		SHIT_GL_400 = majorversion * 10 + minorversion >= 40;
		SHIT_GL_410 = majorversion * 10 + minorversion >= 41;
		SHIT_GL_420 = majorversion * 10 + minorversion >= 42;
		SHIT_GL_430 = majorversion * 10 + minorversion >= 43;
		SHIT_GL_440 = majorversion * 10 + minorversion >= 44;
		SHIT_GL_450 = majorversion * 10 + minorversion >= 45;
		SHIT_GL_460 = majorversion * 10 + minorversion >= 46;

		LOADGL
		LOADWGL
	}
#endif
}