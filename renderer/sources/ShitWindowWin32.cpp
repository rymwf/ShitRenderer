/**
 * @file ShitWindowWin32.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitWindowWin32.hpp"

#include <windowsx.h>

namespace Shit
{
	void WindowWin32::Create()
	{

		//		DISPLAY_DEVICE lpDisplayDevice;
		//		DEVMODE lpDevMode;
		//		DWORD iDevNum{0};
		//		if (EnumDisplayDevices(nullptr, iDevNum, &lpDisplayDevice, 0))
		//		{
		//			EnumDisplaySettings(lpDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &lpDevMode);
		//		}

		const char CLASS_NAME[] = "WindowWin32 Class";

		mHInstance = GetModuleHandle(NULL);
		WNDCLASS wc = {};

		wc.lpfnWndProc = WindowWin32::WindowProc;
		wc.hInstance = mHInstance;
		wc.lpszClassName = CLASS_NAME;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		RegisterClass(&wc);

		mHwnd = CreateWindowEx(
			WS_EX_ACCEPTFILES,										 // Optional window styles.
			CLASS_NAME,												 // Window class
			mCreateInfo.name,										 // Window text
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, // Window style

			// Size and position
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

			NULL,		  // Parent window
			NULL,		  // Menu
			wc.hInstance, // Instance handle
			this		  // Additional application data
		);

		if (mHwnd == NULL)
			THROW("failed to create win32 window");
		//		sWindowMap[mHwnd] = this;

		RECT rect;
		rect.left = mCreateInfo.rect.offset.x;
		rect.top = mCreateInfo.rect.offset.y;
		rect.right = rect.left + static_cast<int>(mCreateInfo.rect.extent.width);
		rect.bottom = rect.top + static_cast<int>(mCreateInfo.rect.extent.height);
		AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, false, WS_EX_ACCEPTFILES);
		SetWindowPos(mHwnd, HWND_NOTOPMOST, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
		ShowWindow(mHwnd, SW_SHOW);
	}

	LRESULT CALLBACK WindowWin32::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		WindowWin32 *pThis = NULL;
		pThis = (WindowWin32 *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		Event ev{};
		ev.pWindow = pThis;
		switch (uMsg)
		{
		case WM_CREATE:
		{
			CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
			pThis = (WindowWin32 *)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);

			ev.value = Event::EventType{WindowCreateEvent{}};
			pThis->mListener.notify(ev);
			break;
		}
		case WM_CLOSE:
			ev.value = Event::EventType{WindowCloseEvent{}};
			pThis->mListener.notify(ev);
			break;
		case WM_DESTROY:
			ev.value = Event::EventType{WindowCloseEvent{}};
			pThis->mListener.notify(ev);
			PostQuitMessage(0);
			break;
		case WM_SIZE:
			pThis->mCreateInfo.rect.extent.width = LOWORD(lParam);
			pThis->mCreateInfo.rect.extent.height = HIWORD(lParam);
			ev.value = Event::EventType{WindowResizeEvent{pThis->mCreateInfo.rect.extent.width, pThis->mCreateInfo.rect.extent.height}};
			pThis->mListener.notify(ev);
			break;
		case WM_MOUSEMOVE:
			ev.modifier = MapKeyModifier(static_cast<uint32_t>(wParam));
			ev.value = Event::EventType{MouseMoveEvent{static_cast<int>(LOWORD(lParam)), static_cast<int>(HIWORD(lParam))}};
			pThis->mListener.notify(ev);
			break;
		case WM_MOUSEWHEEL:
			ev.modifier = MapKeyModifier(LOWORD(wParam));
			ev.value = Event::EventType{MouseWheelEvent{0, int((wParam >> 31) ? ((wParam >> 16) & 0x7fff) - 0x8000 : HIWORD(wParam)) / WHEEL_DELTA}};
			pThis->mListener.notify(ev);
			break;
		case WM_LBUTTONDOWN:
			ev.modifier = MapKeyModifier(static_cast<uint32_t>(wParam));
			ev.value = Event::EventType{MouseButtonEvent{MouseButton::MOUSE_L, PressAction::DOWN}};
			pThis->mListener.notify(ev);
			break;
		case WM_LBUTTONUP:
			ev.modifier = MapKeyModifier(static_cast<uint32_t>(wParam));
			ev.value = Event::EventType{MouseButtonEvent{MouseButton::MOUSE_L, PressAction::UP}};
			pThis->mListener.notify(ev);
			break;
		case WM_RBUTTONDOWN:
			ev.value = Event::EventType{MouseButtonEvent{MouseButton::MOUSE_R, PressAction::DOWN}};
			ev.modifier = MapKeyModifier(static_cast<uint32_t>(wParam));
			pThis->mListener.notify(ev);
			break;
		case WM_RBUTTONUP:
			ev.value = Event::EventType{MouseButtonEvent{MouseButton::MOUSE_R, PressAction::UP}};
			ev.modifier = MapKeyModifier(static_cast<uint32_t>(wParam));
			pThis->mListener.notify(ev);
			break;
		case WM_KEYDOWN:
			if ((lParam >> 24) & 0xff)
			{
				ev.modifier = static_cast<EventModifierBits>(((GetKeyState(VK_SHIFT) < 0) * static_cast<int>(EventModifierBits::SHIFT)) |
															 ((GetKeyState(VK_CONTROL) < 0) * static_cast<int>(EventModifierBits::CTRL)) |
															 ((GetKeyState(VK_MENU) < 0) * static_cast<int>(EventModifierBits::ALT)));
			}
			ev.value = Event::EventType{KeyEvent{MapKey(static_cast<uint32_t>(wParam)), PressAction::DOWN}};
			pThis->mListener.notify(ev);
			break;
		case WM_KEYUP:
			if ((lParam >> 24) & 0xff)
			{
				ev.modifier = static_cast<EventModifierBits>(((GetKeyState(VK_SHIFT) < 0) * static_cast<int>(EventModifierBits::SHIFT)) |
															 ((GetKeyState(VK_CONTROL) < 0) * static_cast<int>(EventModifierBits::CTRL)) |
															 ((GetKeyState(VK_MENU) < 0) * static_cast<int>(EventModifierBits::ALT)));
			}
			ev.value = Event::EventType{KeyEvent{MapKey(static_cast<uint32_t>(wParam)), PressAction::UP}};
			pThis->mListener.notify(ev);
			break;
		case WM_CHAR:
			ev.value = Event::EventType{CharEvent{static_cast<uint32_t>(wParam)}};
			pThis->mListener.notify(ev);
			break;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void WindowWin32::SetSize(uint32_t width, uint32_t height)
	{
		mCreateInfo.rect.extent.width = width;
		mCreateInfo.rect.extent.height = height;
		SetWindowPos(mHwnd, HWND_NOTOPMOST, 0, 0, static_cast<int>(width), static_cast<int>(height), static_cast<int>(SWP_NOMOVE));
	}
	void WindowWin32::SetTitle(const char *title)
	{
		SetWindowText(mHwnd, title);
	}
	void WindowWin32::SetPos(int x, int y)
	{
		mCreateInfo.rect.offset.x = x;
		mCreateInfo.rect.offset.y = y;
		SetWindowPos(mHwnd, HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
	}
	void WindowWin32::Close()
	{
		//PostQuitMessage(0);
		PostMessage(mHwnd, WM_CLOSE, 0, 0);
	}
	bool WindowWin32::PollEvents()
	{
		MSG msg{};
		//while (GetMessage(&msg, NULL, 0, 0))
		//{
		//	TranslateMessage(&msg);
		//	DispatchMessage(&msg);
		//}
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				return false;
		}
		return true;
	}
	void WindowWin32::WaitEvents()
	{
		WaitMessage();
		PollEvents();
		//		MSG msg{};
		//		BOOL bRet;
		//
		//		if ((bRet = GetMessage(&msg, mHwnd, 0, 0)) != 0)
		//		{
		//			if (bRet == -1)
		//			{
		//				// handle the error and possibly exit
		//				THROW("get meesage error");
		//			}
		//			else
		//			{
		//				TranslateMessage(&msg);
		//				DispatchMessage(&msg);
		//			}
		//		}
	}
	void WindowWin32::GetWindowSize(uint32_t &width, uint32_t &height)
	{
		RECT rect{};
		GetWindowRect(mHwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}
	void WindowWin32::GetFramebufferSize(uint32_t &width, uint32_t &height)
	{
		RECT rect{};
		GetClientRect(mHwnd, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}
}
