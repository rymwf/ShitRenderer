/**
 * @file ShitWindow.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitEvent.h"
#include "ShitObserver.h"
#include "ShitSurface.h"
#include "ShitSwapchain.h"

namespace Shit
{
	class ShitWindow
	{
	protected:
		WindowCreateInfo mCreateInfo;
		Observer<const Event &> mObserver;

		std::unique_ptr<Surface> mpSurface;
		std::unique_ptr<Swapchain> mpSwapchain;

	public:
		ShitWindow(const WindowCreateInfo &createInfo) : mCreateInfo(createInfo){};
		const WindowCreateInfo *GetCreateInfo()
		{
			return &mCreateInfo;
		}
		virtual ~ShitWindow() {}

		virtual bool PollEvent() = 0;
		virtual void SetSize(uint32_t width, uint32_t height) = 0;
		virtual void SetTitle(const char *title) = 0;
		virtual void SetPos(int x, int y) = 0;
		virtual void Close() = 0;

		void AttachEventHandle(const std::function<void(const Event &)> &eventHandler)
		{
			mObserver.Attach(eventHandler);
		}
		void SetSurface(std::unique_ptr<Surface> &&surface)
		{
			mpSurface = std::move(surface);
		}
		void SetSwapchain(std::unique_ptr<Swapchain> &&swapchain)
		{
			mpSwapchain = std::move(swapchain);
		}
		Surface* GetSurface() const
		{
			return mpSurface.get();
		}
		Swapchain *GetSwapchain() const
		{
			return mpSwapchain.get();
		}
	};
}