/**
 * @file ShitWin32Window.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "Platform\Windows\ShitWin32Window.h"

#include <windowsx.h>

namespace Shit
{
	void Win32Window::Create()
	{			

//		DISPLAY_DEVICE lpDisplayDevice;
//		DEVMODE lpDevMode;
//		DWORD iDevNum{0};
//		if (EnumDisplayDevices(nullptr, iDevNum, &lpDisplayDevice, 0))
//		{
//			EnumDisplaySettings(lpDisplayDevice.DeviceName, ENUM_CURRENT_SETTINGS, &lpDevMode);
//		}

		const char CLASS_NAME[] = "Win32Window Class";

		mHInstance=GetModuleHandle(NULL);
		WNDCLASS wc = {};

		wc.lpfnWndProc = Win32Window::WindowProc;
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
	}

	LRESULT CALLBACK Win32Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Win32Window *pThis = NULL;
		pThis = (Win32Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (uMsg == WM_CREATE)
		{
			CREATESTRUCT *pCreate = (CREATESTRUCT *)lParam;
			pThis = (Win32Window *)pCreate->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
		}
		else if (pThis)
		{
			Event ev{};
			switch (uMsg)
			{
			case WM_CLOSE:
			case WM_DESTROY:
				ev.type = EventType::WINDOW_CLOSE;
				ev.windowClose.pWindow = pThis;
				pThis->mObserver.Notify(ev);
				PostQuitMessage(0);
				return 0;
			case WM_SIZE:
				ev.type = EventType::WINDOW_RESIZE;
				pThis->mCreateInfo.rect.extent.width = ev.windowResize.width = LOWORD(lParam);
				pThis->mCreateInfo.rect.extent.height = ev.windowResize.height = HIWORD(lParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_MOUSEMOVE:
				ev.type = EventType::MOUSEMOVE;
				ev.mouseMove.xpos = LOWORD(lParam);
				ev.mouseMove.ypos = HIWORD(lParam);
				ev.modifier = MapKeyModifier(wParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_MOUSEWHEEL:
				ev.type = EventType::MOUSEWHEEL;
				ev.mouseWheel.yoffset = int((wParam >> 31) ? ((wParam >> 16) & 0x7fff) - 0x8000 : HIWORD(wParam)) / WHEEL_DELTA;
				ev.modifier = MapKeyModifier(LOWORD(wParam));
				pThis->mObserver.Notify(ev);
				break;
			case WM_LBUTTONDOWN:
				ev.type = EventType::MOUSEBUTTON;
				ev.mouseButton.action = PressAction::DOWN;
				ev.mouseButton.button = MouseButton::MOUSE_L;
				ev.modifier = MapKeyModifier(wParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_LBUTTONUP:
				ev.type = EventType::MOUSEBUTTON;
				ev.mouseButton.action = PressAction::UP;
				ev.mouseButton.button = MouseButton::MOUSE_L;
				ev.modifier = MapKeyModifier(wParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_RBUTTONDOWN:
				ev.type = EventType::MOUSEBUTTON;
				ev.mouseButton.action = PressAction::DOWN;
				ev.mouseButton.button = MouseButton::MOUSE_R;
				ev.modifier = MapKeyModifier(wParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_RBUTTONUP:
				ev.type = EventType::MOUSEBUTTON;
				ev.mouseButton.action = PressAction::UP;
				ev.mouseButton.button = MouseButton::MOUSE_R;
				ev.modifier = MapKeyModifier(wParam);
				pThis->mObserver.Notify(ev);
				break;
			case WM_KEYDOWN:
				ev.type = EventType::KEYBOARD;
				if ((lParam >> 24) & 0xff)
				{
					ev.modifier = static_cast<EventModifierBits>(((GetKeyState(VK_SHIFT) < 0) * static_cast<int>(EventModifierBits::SHIFT)) |
																 ((GetKeyState(VK_CONTROL) < 0) * static_cast<int>(EventModifierBits::CTRL)) |
																 ((GetKeyState(VK_MENU) < 0) * static_cast<int>(EventModifierBits::ALT)));
				}
				ev.key.key = MapKey(wParam);
				ev.key.action = PressAction::DOWN;
				if (wParam == VK_ESCAPE)
				{
					ev.type = EventType::WINDOW_CLOSE;
					pThis->mObserver.Notify(ev);
					PostQuitMessage(0);
					return 0;
				}
				pThis->mObserver.Notify(ev);
				break;
			case WM_KEYUP:
				ev.type = EventType::KEYBOARD;
				if ((lParam >> 24) & 0xff)
				{
					ev.modifier = static_cast<EventModifierBits>(((GetKeyState(VK_SHIFT) < 0) * static_cast<int>(EventModifierBits::SHIFT)) |
																 ((GetKeyState(VK_CONTROL) < 0) * static_cast<int>(EventModifierBits::CTRL)) |
																 ((GetKeyState(VK_MENU) < 0) * static_cast<int>(EventModifierBits::ALT)));
				}
				ev.key.key = MapKey(wParam);
				ev.key.action = PressAction::UP;
				pThis->mObserver.Notify(ev);
				break;
			case WM_CHAR:
				ev.type = EventType::CHAR;
				ev.charInput.codepoint = wParam;
				pThis->mObserver.Notify(ev);
				break;
			}
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	void Win32Window::SetSize(uint32_t width, uint32_t height)
	{
		mCreateInfo.rect.extent.width = width;
		mCreateInfo.rect.extent.height = height;
		SetWindowPos(mHwnd, HWND_NOTOPMOST, 0, 0, static_cast<int>(width), static_cast<int>(height), static_cast<int>(SWP_NOMOVE));
	}
	void Win32Window::SetTitle(const char *title)
	{
		SetWindowText(mHwnd, title);
	}
	void Win32Window::SetPos(int x, int y)
	{
		mCreateInfo.rect.offset.x = x;
		mCreateInfo.rect.offset.y = y;
		SetWindowPos(mHwnd, HWND_NOTOPMOST, x, y, 0, 0, SWP_NOSIZE);
	}
	//void Win32Window::Close()
	//{
	//}
	void Win32Window::PollEvent()
	{
		MSG msg{};
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}
