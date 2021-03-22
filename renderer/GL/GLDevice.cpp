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
#include "GLSwapchain.h"
#include "GLShader.h"
#include "GLCommandBuffer.h"
#include "GLBuffer.h"
#include "GLImage.h"
#include "GLDescriptor.h"
#include "GLSampler.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{
	static void APIENTRY DebugOutputCallback(GLenum source, GLenum type, GLuint id,
											 GLenum severity, GLsizei length,
											 const GLchar *message,
											 const void *userParam)
	{
		if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
			return; // ignore these non-significant error codes
		std::stringstream sstr;
		sstr << "Debug message id:" << id << " length:" << length
			 << " useParam:" << userParam << " message:" << message << "\n";
		//		LOG_DEBUG(":{}, message:{}", id, message);
		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			sstr << "Source: API :0x" << std::hex << GL_DEBUG_SOURCE_API;
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			sstr << "Source: Window System :0x" << std::hex
				 << GL_DEBUG_SOURCE_WINDOW_SYSTEM;
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			sstr << "Source: Shader Compiler :0x" << std::hex
				 << GL_DEBUG_SOURCE_SHADER_COMPILER;
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			sstr << "Source: Third Party :0x" << std::hex
				 << GL_DEBUG_SOURCE_THIRD_PARTY;
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			sstr << "Source: Application :0x" << std::hex
				 << GL_DEBUG_SOURCE_APPLICATION;
			break;
		case GL_DEBUG_SOURCE_OTHER:
			sstr << "Source: Other :0x" << std::hex << GL_DEBUG_SOURCE_OTHER;
			break;
		}
		sstr << "\n";
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			sstr << "Type: Error :0x" << std::hex << GL_DEBUG_TYPE_ERROR;
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			sstr << "Type: Deprecated Behaviour :0x" << std::hex
				 << GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR;
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			sstr << "Type: Undefined Behaviour :0x" << std::hex
				 << GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR;
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			sstr << "Type: Portability :0x" << std::hex << GL_DEBUG_TYPE_PORTABILITY;
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			sstr << "Type: Performance :0x" << std::hex << GL_DEBUG_TYPE_PERFORMANCE;
			break;
		case GL_DEBUG_TYPE_MARKER:
			sstr << "Type: Marker :0x" << std::hex << GL_DEBUG_TYPE_MARKER;
			break;
		case GL_DEBUG_TYPE_PUSH_GROUP:
			sstr << "Type: Push Group :0x" << std::hex << GL_DEBUG_TYPE_PUSH_GROUP;
			break;
		case GL_DEBUG_TYPE_POP_GROUP:
			sstr << "Type: Pop Group :0x" << std::hex << GL_DEBUG_TYPE_POP_GROUP;
			break;
		case GL_DEBUG_TYPE_OTHER:
			sstr << "Type: Other :0x" << std::hex << GL_DEBUG_TYPE_OTHER;
			break;
		}

		sstr << "\n";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			sstr << "Severity: high :0x" << std::hex << GL_DEBUG_SEVERITY_HIGH;
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			sstr << "Severity: medium :0x" << std::hex << GL_DEBUG_SEVERITY_MEDIUM;
			break;
		case GL_DEBUG_SEVERITY_LOW:
			sstr << "Severity: low :0x" << std::hex << GL_DEBUG_SEVERITY_LOW;
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			sstr << "Severity: notification :0x" << std::hex
				 << GL_DEBUG_SEVERITY_NOTIFICATION;
			break;
		}
		LOG(sstr.str());
	}
	void GLDevice::EnableDebugOutput(const void *userParam)
	{
		// enable OpenGL debug context if context allows for debug context
		GLint flags;
		glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
		if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
		{
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed
												   // synchronously
			if (GLEW_VERSION_4_3)
			{
				glDebugMessageCallback(DebugOutputCallback, userParam);
				glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
									  NULL, GL_TRUE);
			}
			else
			{
#ifdef GL_ARB_debug_output
				glDebugMessageCallbackARB(DebugOutputCallback, userParam);
				glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0,
										 NULL, GL_TRUE);
#else
				LOG("current opengl version do not support debug output");
#endif
			}
		}
		else
		{
			LOG("current context do not support debug output");
		}
	}
	CommandPool *GLDevice::Create(const CommandPoolCreateInfo &createInfo)
	{
		mCommandPools.emplace_back(std::make_unique<GLCommandPool>(&mStateManager, createInfo));
		return mCommandPools.back().get();
	}

	Shader *GLDevice::Create(const ShaderCreateInfo &createInfo)
	{
		mShaders.emplace_back(std::make_unique<GLShader>(createInfo));
		return mShaders.back().get();
	}

	Pipeline *GLDevice::Create(const GraphicsPipelineCreateInfo &createInfo)
	{
		mPipelines.emplace_back(std::make_unique<GLGraphicsPipeline>(&mStateManager, createInfo));
		return mPipelines.back().get();
	}

	Queue *GLDevice::Create(const QueueCreateInfo &createInfo)
	{
		mQueues.emplace_back(std::make_unique<GLQueue>(this, &mStateManager, createInfo));
		return mQueues.back().get();
	}
	Buffer *GLDevice::Create(const BufferCreateInfo &createInfo, void *pData)
	{
		mBuffers.emplace_back(std::make_unique<GLBuffer>(&mStateManager, createInfo, pData));
		return mBuffers.back().get();
	}

	Image *GLDevice::Create(const ImageCreateInfo &createInfo, void *pData)
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
	DescriptorSetLayout *GLDevice::Create(const DescriptorSetLayoutCreateInfo &createInfo)
	{
		mDescriptorSetLayouts.emplace_back(std::make_unique<GLDescriptorSetLayout>(createInfo));
		return mDescriptorSetLayouts.back().get();
	}

	ImageView *GLDevice::Create(const ImageViewCreateInfo &createInfo)
	{
		mImageViews.emplace_back(std::make_unique<GLImageView>(&mStateManager, createInfo));
		return mImageViews.back().get();
	}
	PipelineLayout *GLDevice::Create(const PipelineLayoutCreateInfo &createInfo)
	{
		mPipelineLayouts.emplace_back(std::make_unique<GLPipelineLayout>(createInfo));
		return mPipelineLayouts.back().get();
	}
	RenderPass *GLDevice::Create(const RenderPassCreateInfo &createInfo)
	{
		mRenderPasses.emplace_back(std::make_unique<GLRenderPass>(createInfo));
		return mRenderPasses.back().get();
	}
	Framebuffer *GLDevice::Create(const FramebufferCreateInfo &createInfo)
	{
		mFramebuffers.emplace_back(std::make_unique<GLFramebuffer>(&mStateManager, createInfo));
		return mFramebuffers.back().get();
	}
	Semaphore *GLDevice::Create(const SemaphoreCreateInfo &createInfo)
	{
		mSemaphores.emplace_back(std::make_unique<GLSemaphore>(&mStateManager, createInfo));
		return mSemaphores.back().get();
	}
	Fence *GLDevice::Create(const FenceCreateInfo &createInfo)
	{
		mFences.emplace_back(std::make_unique<GLFence>(&mStateManager, createInfo));
		return mFences.back().get();
	}
	Swapchain *GLDevice::Create(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow)
	{
		if (pWindow != std::get<ShitWindow *>(mCreateInfo.physicalDevice))
			THROW("you should use a new device to create swapchain for a new window");

		mSwapchains.emplace_back(std::make_unique<GLSwapchain>(this, &mStateManager, createInfo));
		auto pSwapchain = mSwapchains.back().get();
		pWindow->SetSwapchain(pSwapchain);
		pWindow->AddEventListener(static_cast<GLSwapchain *>(pSwapchain)->GetProcessWindowEventCallable());
		return pSwapchain;
	}

#ifdef _WIN32
	void GLDeviceWin32::InitWglExtentions()
	{
		WNDCLASSA window_class = {
			.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
			.lpfnWndProc = DefWindowProcA,
			.hInstance = GetModuleHandle(0),
			.lpszClassName = "Dummy_WGL_djuasiodwa",
		};

		if (!RegisterClassA(&window_class))
		{
			THROW("Failed to register dummy OpenGL window.");
		}

		HWND dummy_window = CreateWindowExA(
			0,
			window_class.lpszClassName,
			"Dummy OpenGL Window",
			0,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			window_class.hInstance,
			0);

		if (!dummy_window)
		{
			THROW("Failed to create dummy OpenGL window.");
		}

		HDC dummy_dc = GetDC(dummy_window);

		PIXELFORMATDESCRIPTOR pfd = {
			.nSize = sizeof(pfd),
			.nVersion = 1,
			.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 32,
			.cAlphaBits = 8,
			.cDepthBits = 24,
			.cStencilBits = 8,
			.iLayerType = PFD_MAIN_PLANE,
		};
		//PIXELFORMATDESCRIPTOR pfd = {
		//	sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
		//	1,							   // version number
		//	PFD_DRAW_TO_WINDOW |		   // support window
		//		PFD_SUPPORT_OPENGL |	   // support OpenGL
		//		PFD_DOUBLEBUFFER,		   // double buffered
		//	PFD_TYPE_RGBA,				   // RGBA type
		//	32,							   // 32-bit color depth framebuffer bits
		//	0, 0, 0, 0, 0, 0,			   // color bits ignored
		//	0,							   // no alpha buffer
		//	0,							   // shift bit ignored
		//	0,							   // no accumulation buffer
		//	0, 0, 0, 0,					   // accum bits ignored
		//	24,							   // 24-bit z-buffer
		//	8,							   // 8-bit stencil buffer
		//	0,							   // no auxiliary buffer
		//	PFD_MAIN_PLANE,				   // main layer
		//	0,							   // reserved
		//	0, 0, 0						   // layer masks ignored
		//};

		int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
		if (!pixel_format)
		{
			THROW("Failed to find a suitable pixel format.");
		}
		if (!SetPixelFormat(dummy_dc, pixel_format, &pfd))
		{
			THROW("Failed to set the pixel format.");
		}

		HGLRC dummy_context = wglCreateContext(dummy_dc);
		if (!dummy_context)
		{
			THROW("Failed to create a dummy OpenGL rendering context.");
		}

		if (!wglMakeCurrent(dummy_dc, dummy_context))
		{
			THROW("Failed to activate dummy OpenGL rendering context.");
		}

		//LOADWGL
		//wglCreateContextAttribsARB = wglGetProcAddress("wglCreateContextAttribsARB");
		wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
		wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

		wglMakeCurrent(dummy_dc, 0);
		wglDeleteContext(dummy_context);
		ReleaseDC(dummy_window, dummy_dc);
		DestroyWindow(dummy_window);
	}

	void GLDeviceWin32::CreateRenderContext()
	{
		const int attribList[] =
			{
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
				WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, //or WGL_TYPE_COLORINDEX_ARB
				WGL_COLOR_BITS_ARB, 32,
				WGL_DEPTH_BITS_ARB, 24,
				WGL_STENCIL_BITS_ARB, 8,
				WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
				0, // End
			};

		int pixelFormat{};
		UINT numFormats;
		if (!wglChoosePixelFormatARB(mHDC, attribList, NULL, 1, &pixelFormat, &numFormats))
		{
			THROW("wglChoosePixelFormatARB failed");
		}
		LOG_VAR(pixelFormat);

		int majorversion = (std::max)((static_cast<int>(mRenderSystemCreateInfo.version) >> 2) & 1, 1);
		int minorversion = (static_cast<int>(mRenderSystemCreateInfo.version) >> 1) & 1;

		int contexFlag{};
		if (static_cast<bool>(mRenderSystemCreateInfo.flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
			contexFlag |= WGL_CONTEXT_DEBUG_BIT_ARB;
		if (static_cast<bool>(mRenderSystemCreateInfo.flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_FORWARD_COMPATIBLE_BIT))
			contexFlag |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		int profileMaskFlag{WGL_CONTEXT_CORE_PROFILE_BIT_ARB};
		if (static_cast<bool>(mRenderSystemCreateInfo.flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
			profileMaskFlag |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
		LOG_VAR(majorversion);
		LOG_VAR(minorversion);
		const int attribList2[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,
			majorversion, //seems to be not support < 330
			WGL_CONTEXT_MINOR_VERSION_ARB,
			minorversion,
			WGL_CONTEXT_FLAGS_ARB, contexFlag,
			WGL_CONTEXT_PROFILE_MASK_ARB,
			profileMaskFlag,
			0};

		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat(mHDC, pixelFormat, sizeof(pfd), &pfd);
		if (!SetPixelFormat(mHDC, pixelFormat, &pfd))
		{
			THROW("Failed to set the OpenGL 3.3 pixel format.");
		}

		mHRenderContext = wglCreateContextAttribsARB(mHDC, 0, attribList2);
		if (!mHRenderContext)
		{
			THROW("failed to create render context");
		}
		LOG("create render context succeed");
		MakeCurrent();

		LOADGL

		if (static_cast<bool>(mRenderSystemCreateInfo.flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
			EnableDebugOutput(this);
		//
		glGetIntegerv(GL_MAJOR_VERSION, &GLVersion.major);
		glGetIntegerv(GL_MINOR_VERSION, &GLVersion.minor);

		LOG_VAR(GLVersion.major);
		LOG_VAR(GLVersion.minor);

		LOG_VAR(GL::queryWGLExtensionNames(mHDC));
	}

	void GLDeviceWin32::SetPresentMode(PresentMode mode) const
	{
		if (mode == PresentMode::IMMEDIATE)
			wglSwapIntervalEXT(0);
		else if (mode == PresentMode::FIFO)
			wglSwapIntervalEXT(1);
	}

	void GLDeviceWin32::MakeCurrent() const
	{
		//if (!wglMakeContextCurrentARB(mHDC, mHDC, mHRenderContext))
		if (!wglMakeCurrent(mHDC, mHRenderContext))
		{
			LOG_VAR(GetLastError());
			THROW("failed to make context current");
		}
	}
	void GLDeviceWin32::SwapBuffer() const
	{
		SwapBuffers(mHDC);
	}
	void GLDeviceWin32::GetWindowPixelFormats([[maybe_unused]] const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats)
	{
		formats.clear();
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
	void GLDeviceWin32::GetPresentModes([[maybe_unused]] const ShitWindow *pWindow, std::vector<PresentMode> &presentModes)
	{
		presentModes.clear();
		presentModes.emplace_back(PresentMode::IMMEDIATE);
		presentModes.emplace_back(PresentMode::FIFO);
	}
	GLDeviceWin32::GLDeviceWin32(const DeviceCreateInfo &createInfo, const RenderSystemCreateInfo &renderSystemCreateInfo)
		: GLDevice(createInfo, renderSystemCreateInfo)
	{
		InitWglExtentions();

		mHDC = GetDC(static_cast<WindowWin32 *>(std::get<ShitWindow *>(createInfo.physicalDevice))->GetHWND());
		if (!mHDC)
			THROW("failed to create window context");

		CreateRenderContext();

		SHIT_GL_110 = GLVersion.major * 10 + GLVersion.minor >= 11;
		SHIT_GL_120 = GLVersion.major * 10 + GLVersion.minor >= 12;
		SHIT_GL_121 = GLVersion.major * 10 + GLVersion.minor >= 13;
		SHIT_GL_130 = GLVersion.major * 10 + GLVersion.minor >= 13;
		SHIT_GL_140 = GLVersion.major * 10 + GLVersion.minor >= 14;
		SHIT_GL_150 = GLVersion.major * 10 + GLVersion.minor >= 15;
		SHIT_GL_200 = GLVersion.major * 10 + GLVersion.minor >= 20;
		SHIT_GL_210 = GLVersion.major * 10 + GLVersion.minor >= 21;
		SHIT_GL_300 = GLVersion.major * 10 + GLVersion.minor >= 30;
		SHIT_GL_310 = GLVersion.major * 10 + GLVersion.minor >= 31;
		SHIT_GL_320 = GLVersion.major * 10 + GLVersion.minor >= 32;
		SHIT_GL_330 = GLVersion.major * 10 + GLVersion.minor >= 33;
		SHIT_GL_400 = GLVersion.major * 10 + GLVersion.minor >= 40;
		SHIT_GL_410 = GLVersion.major * 10 + GLVersion.minor >= 41;
		SHIT_GL_420 = GLVersion.major * 10 + GLVersion.minor >= 42;
		SHIT_GL_430 = GLVersion.major * 10 + GLVersion.minor >= 43;
		SHIT_GL_440 = GLVersion.major * 10 + GLVersion.minor >= 44;
		SHIT_GL_450 = GLVersion.major * 10 + GLVersion.minor >= 45;
		SHIT_GL_460 = GLVersion.major * 10 + GLVersion.minor >= 46;
		mStateManager.UpdateCapbilityState();
	}
#endif
}