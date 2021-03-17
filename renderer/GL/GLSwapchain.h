/**
 * @file GLSwapchain.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSwapchain.h>
#include "GLPrerequisites.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{

	/**
	 * @brief images count is in [8,16]
	 * 
	 */
	class GLSwapchain : public Swapchain
	{
		Framebuffer *mpFramebuffer{};
		GLDevice *mpDevice;
		GLStateManager *mpStateManager;
		uint32_t mAvailableImageIndex{};

	protected:
		void CreateImages(uint32_t count);

		/**
		 * @brief Create a Framebuffer object used to blit
		 * 
		 */
		void CreateFramebuffer();

		constexpr uint32_t GetSwapchainImageCount() const
		{
			return (std::min)((std::max)(mCreateInfo.minImageCount, 8U), 16U);
		}

		void EnableDebugOutput(const void *userParam);

	public:
		GLSwapchain(GLDevice *pDevice, GLStateManager *pStateManager, const SwapchainCreateInfo &createInfo)
			: Swapchain(createInfo), mpDevice(pDevice), mpStateManager(pStateManager) {}

		~GLSwapchain() override;

		uint32_t GetNextImage(const GetNextImageInfo &info) override;

		virtual void SwapBuffer() = 0;

		constexpr const Framebuffer *GetFramebufferPtr() const
		{
			return mpFramebuffer;
		}
	};

#ifdef _WIN32
	class GLSwapchainWin32 final : public GLSwapchain
	{
		HGLRC mHglrc;
		HDC mHdc;

	public:
		GLSwapchainWin32(GLDevice *pDevice,
						 GLStateManager *pStateManager,
						 HDC hdc,
						 const SwapchainCreateInfo &createInfo,
						 RendererVersion version,
						 RenderSystemCreateFlagBits flags);

		~GLSwapchainWin32() override
		{
			wglDeleteContext(mHglrc);
		}

		constexpr HGLRC GetHandle() const
		{
			return mHglrc;
		}
		void MakeCurrent() const
		{
			if (!wglMakeContextCurrentARB(mHdc, mHdc, mHglrc))
			{
				LOG_VAR(GetLastError());
				THROW("failed to make context current");
			}
		}
		void SwapBuffer()
		{
			SwapBuffers(mHdc);
		}
	};
#endif

} // namespace Shit
