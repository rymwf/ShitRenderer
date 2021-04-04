/**
 * @file ShitWindow.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitEvent.hpp"
#include "ShitListener.hpp"

namespace Shit
{
	//TODO: should destroy swapchain when closed
	class ShitWindow
	{
	protected:
		WindowCreateInfo mCreateInfo;
		Listener<const Event &> mListener;

		Surface *mpSurface{};
		Swapchain *mpSwapchain{};
		RenderSystem *mpRenderSystem;

		void SetSurface(Surface *pSurface)
		{
			mpSurface = pSurface;
		}

	public:
		ShitWindow(const WindowCreateInfo &createInfo, RenderSystem *pRenderSystem) : mCreateInfo(createInfo), mpRenderSystem(pRenderSystem)
		{
			if (createInfo.eventListener)
				AddEventListener(createInfo.eventListener);
		}
		const WindowCreateInfo *GetCreateInfo()
		{
			return &mCreateInfo;
		}
		virtual ~ShitWindow() {}

		virtual bool PollEvents() = 0;
		virtual void WaitEvents() = 0;
		virtual void SetSize(uint32_t width, uint32_t height) = 0;
		virtual void SetTitle(const char *title) = 0;
		virtual void SetPos(int x, int y) = 0;
		virtual void Close() = 0;
		virtual void GetWindowSize(uint32_t &width, uint32_t& height) = 0;
		virtual void GetFramebufferSize(uint32_t &width, uint32_t &height) = 0;

		void AddEventListener(const std::shared_ptr<std::function<void(const Event &)>> &eventListener)
		{
			mListener.emplace_back(eventListener);
		}
		void SetSwapchain(Swapchain *pSwapchain)
		{
			mpSwapchain = pSwapchain;
		}
		constexpr const Surface *GetSurfacePtr() const
		{
			return mpSurface;
		}
		constexpr const Swapchain *GetSwapchainPtr() const
		{
			return mpSwapchain;
		}
		friend class RenderSystem;
	};
}