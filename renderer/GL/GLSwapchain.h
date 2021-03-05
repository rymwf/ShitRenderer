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
#include "GLDevice.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{
#ifdef _WIN32
	class GLSwapchainWin32 final : public Swapchain
	{
		HGLRC mHglrc;
		HDC mHdc;

	public:
		GLSwapchainWin32(HDC hdc, const SwapchainCreateInfo &createInfo, RendererVersion version, RenderSystemCreateFlagBits flags);

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
				fprintf(stderr, "ErrorCode: (0x)%x/n", GetLastError());
				THROW("failed to make context current");
			}
		}
	};
#endif

} // namespace Shit
