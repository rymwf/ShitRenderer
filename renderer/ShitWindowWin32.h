/**
 * @file ShitWin32Window.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitWindow.h"
#include <Windows.h>

namespace Shit
{
	class WindowWin32 final : public ShitWindow
	{
		HWND mHwnd;
		HINSTANCE mHInstance;

		void Create();

	public:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		WindowWin32(const WindowCreateInfo &createInfo, RenderSystem *pRenderSystem) : ShitWindow(createInfo, pRenderSystem)
		{
			Create();
		}
		~WindowWin32() override {}

		void SetSize(uint32_t width, uint32_t height) override;
		void SetTitle(const char *title) override;
		void SetPos(int x, int y) override;
		void Close() override;

		bool PollEvent() override;
		HWND GetHWND() const { return mHwnd; }
		HINSTANCE GetInstance() const { return mHInstance; }
	};
}