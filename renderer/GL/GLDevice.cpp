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

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{

	Shader *GLDevice::CreateShader( const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<GLShader>(&mStateManager, createInfo));
		return mShaders.back().get();
	}

	void GLDevice::DestroyShader(Shader *pShader)
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

	GraphicsPipeline *GLDevice::CreateGraphicsPipeline( const GraphicsPipelineCreateInfo &createInfo)
	{
		mGraphicsPipelines.emplace_back(std::make_unique<GLGraphicsPipeline>(&mStateManager, createInfo));
		return mGraphicsPipelines.back().get();
	}
	CommandBuffer *GLDevice::CreateCommandBuffer( const CommandBufferCreateInfo &createInfo)
	{
		mCommandBuffers.emplace_back(std::make_unique<GLCommandBuffer>(&mStateManager, createInfo));
		return mCommandBuffers.back().get();
	}

	Queue *GLDevice::CreateDeviceQueue( const QueueCreateInfo &createInfo)
	{
		return nullptr;
	}
	Result GLDevice::WaitForFence( Fence *fence, uint64_t timeout)
	{
		return Result::SUCCESS;
	}
	Buffer *GLDevice::CreateBuffer( const BufferCreateInfo &createInfo,void* pData)
	{
		mBuffers.emplace_back(std::make_unique<GLBuffer>(&mStateManager, createInfo, pData));
		return mBuffers.back().get();
	}
#ifdef _WIN32

	Swapchain* GLDeviceWin32::CreateSwapchain(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		pWindow->SetSwapchain(std::make_shared<GLSwapchainWin32>(mHDC, createInfo, mRenderSystemCreatInfo.version, mRenderSystemCreatInfo.flags));
		return pWindow->GetSwapchain();
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