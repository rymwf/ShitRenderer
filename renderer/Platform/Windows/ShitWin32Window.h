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
	class Win32Window final : public ShitWindow
	{
		HWND mHwnd;
		HINSTANCE mHInstance;

		void Create();

	public:
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		Win32Window(const WindowCreateInfo &createInfo) : ShitWindow(createInfo)
		{
			Create();
		}
		~Win32Window() override {}

		void SetSize(uint32_t width, uint32_t height) override;
		void SetTitle(const char *title) override;
		void SetPos(int x, int y) override;

		//void Close() override;
		void PollEvent() override;
		void *GetNativeHandle() const override { return mHwnd; }
		void *GetNativeInstance() const override { return mHInstance; }
	};
}