/**
 * @file GLSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLSwapchain.h"
#include "GLImage.h"
#include "GLDevice.h"
#include "GLFence.h"
#include "GLSemaphore.h"
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
	void GLSwapchain::EnableDebugOutput(const void *userParam)
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
	GLSwapchain::~GLSwapchain()
	{
		mpDevice->Destroy(mpFramebuffer);
	}
	void GLSwapchain::CreateImages(uint32_t count)
	{
		ImageCreateInfo imageCreateInfo{
			{},
			ImageType::TYPE_2D,
			mCreateInfo.format,
			{mCreateInfo.imageExtent.width,
			 mCreateInfo.imageExtent.height,
			 1u},
			1,
			1,
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		while (count-- > 0)
		{
			mImages.emplace_back(std::make_unique<GLImage>(mpStateManager, imageCreateInfo));
		}
	}
	void GLSwapchain::CreateFramebuffer()
	{
		std::vector<ImageView *> imageViews;
		ImageViewCreateInfo imageViewCreateInfo{nullptr,
												ImageViewType::TYPE_2D,
												mCreateInfo.format,
												{},
												{0, 1, 0, 1}};
		for (auto &&e : mImages)
		{
			imageViewCreateInfo.pImage = e.get();
			imageViews.emplace_back(mpDevice->CreateImageView(imageViewCreateInfo));
		}
		FramebufferCreateInfo createInfo{nullptr,
										 imageViews,
										 createInfo.extent,
										 1};
		mpFramebuffer = mpDevice->CreateFramebuffer(createInfo);
	}

	uint32_t GLSwapchain::GetNextImage(const GetNextImageInfo &info)
	{
		if (info.pFence)
			static_cast<GLFence *>(info.pFence)->Reset();
		if (info.pSemaphore)
			static_cast<GLSemaphore *>(info.pSemaphore)->Reset();
		mAvailableImageIndex %= mImages.size();
		return mAvailableImageIndex++;
	}

#ifdef _WIN32
	GLSwapchainWin32::GLSwapchainWin32(
		GLDevice *pDevice,
		GLStateManager *pStateManager,
		HDC hdc,
		const SwapchainCreateInfo &createInfo,
		RendererVersion version,
		RenderSystemCreateFlagBits flags)
		: GLSwapchain(pDevice, pStateManager, createInfo), mHdc(hdc)
	{
		auto colorFormat = ShitFormat::RGBA8_SRGB;
		if (createInfo.format == ShitFormat::RGBA8_UNORM)
			colorFormat = ShitFormat::RGBA8_UNORM;
		mCreateInfo.format = colorFormat;

		const int attribList[] =
			{
				WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				//WGL_ACCELERATION_ARB,WGL_FULL_ACCELERATION_ARB,
				WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB, //or WGL_TYPE_COLORINDEX_ARB
				WGL_COLOR_BITS_ARB, 32,
				WGL_DEPTH_BITS_ARB, 24,
				WGL_STENCIL_BITS_ARB, 8,
				WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
				0, // End
			};

		int pixelFormat{};
		UINT numFormats;
		if (!wglChoosePixelFormatARB(mHdc, attribList, NULL, 1, &pixelFormat, &numFormats))
		{
			THROW("wglChoosePixelFormatARB failed");
		}
		LOG_VAR(pixelFormat);

		int majorversion = (std::max)((static_cast<int>(version) >> 2) & 1, 1);
		int minorversion = (static_cast<int>(version) >> 1) & 1;

		int contexFlag{};
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT))
			contexFlag |= WGL_CONTEXT_DEBUG_BIT_ARB;
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_FORWARD_COMPATIBLE_BIT))
			contexFlag |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
		int profileMaskFlag{WGL_CONTEXT_CORE_PROFILE_BIT_ARB};
		if (static_cast<bool>(flags & RenderSystemCreateFlagBits::SHIT_GL_CONTEXT_COMPATIBILITY_PROFILE_BIT))
			profileMaskFlag |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;

		const int attribList2[] = {
			WGL_CONTEXT_MAJOR_VERSION_ARB,
			majorversion, //seems to be not support < 330
			WGL_CONTEXT_MINOR_VERSION_ARB,
			minorversion,
			WGL_CONTEXT_FLAGS_ARB, contexFlag,
			WGL_CONTEXT_PROFILE_MASK_ARB,
			profileMaskFlag,
			0};

		mHglrc = wglCreateContextAttribsARB(mHdc, 0, attribList2);
		if (!mHglrc)
		{
			THROW("failed to create render context");
		}

		MakeCurrent();

		if (createInfo.presentMode == PresentMode::IMMEDIATE)
			wglSwapIntervalEXT(0);
		else if (createInfo.presentMode == PresentMode::FIFO)
			wglSwapIntervalEXT(1);

		//reinit opengl and wgl
		LOADGL
		LOADWGL

		EnableDebugOutput(this);

		if (colorFormat == ShitFormat::RGBA8_SRGB)
			glEnable(GL_FRAMEBUFFER_SRGB);

		//
		glGetIntegerv(GL_MAJOR_VERSION, &GLVersion.major);
		glGetIntegerv(GL_MINOR_VERSION, &GLVersion.minor);

		LOG_VAR(GLVersion.major);
		LOG_VAR(GLVersion.minor);

		LOG_VAR(GL::queryWGLExtensionNames(mHdc));
#ifndef NDEBUG
		GL::listGLInfo();
#endif
		CreateImages(GetSwapchainImageCount());
		CreateFramebuffer();
	}
#endif
} // namespace Shit
