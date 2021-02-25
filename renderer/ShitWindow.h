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
#include "ShitContext.h"

namespace Shit
{
	class ShitWindow
	{
	protected:
		WindowCreateInfo mCreateInfo;
		Observer<const Event &> mObserver;

	public:
		ShitWindow(const WindowCreateInfo &createInfo) : mCreateInfo(createInfo){};
		const WindowCreateInfo *GetCreateInfo()
		{
			return &mCreateInfo;
		}
		virtual ~ShitWindow() {}

		virtual void PollEvent() = 0;
		virtual void *GetNativeHandle() const= 0;
		virtual void *GetNativeInstance() const= 0;
		virtual void SetSize(uint32_t width, uint32_t height) = 0;
		virtual void SetTitle(const char *title) = 0;
		virtual void SetPos(int x, int y) = 0;
	};
}