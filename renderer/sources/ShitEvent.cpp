/**
 * @file ShitEvent.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitEvent.h"
#if _WIN32
#include <Windows.h>
#endif
namespace Shit
{

#if _WIN32

	static uint32_t shitKey2NativeKeyMap[]{
		VK_SPACE,
		VK_OEM_7,
		VK_OEM_COMMA,
		VK_OEM_MINUS,
		VK_OEM_PERIOD,
		VK_OEM_2,
		('0'),
		('1'),
		('2'),
		('3'),
		('4'),
		('5'),
		('6'),
		('7'),
		('8'),
		('9'),
		VK_OEM_1,
		VK_OEM_PLUS,
		('A'),
		('B'),
		('C'),
		('D'),
		('E'),
		('F'),
		('G'),
		('H'),
		('I'),
		('J'),
		('K'),
		('L'),
		('M'),
		('N'),
		('O'),
		('P'),
		('Q'),
		('R'),
		('S'),
		('T'),
		('U'),
		('V'),
		('W'),
		('X'),
		('Y'),
		('Z'),
		VK_OEM_4,
		VK_OEM_5,
		VK_OEM_6,
		VK_OEM_3,
		// GLFW_KEY_WORLD_1,
		// GLFW_KEY_WORLD_2,

		VK_ESCAPE,
		VK_RETURN,
		VK_TAB,
		VK_BACK,
		VK_INSERT,
		VK_DELETE,
		VK_RIGHT,
		VK_LEFT,
		VK_DOWN,
		VK_UP,
		VK_PRIOR,
		VK_NEXT,
		VK_HOME,
		VK_END,
		VK_CAPITAL,
		VK_SCROLL,
		VK_NUMLOCK,
		VK_SNAPSHOT,
		VK_PAUSE,
		VK_F1,
		VK_F2,
		VK_F3,
		VK_F4,
		VK_F5,
		VK_F6,
		VK_F7,
		VK_F8,
		VK_F9,
		VK_F10,
		VK_F11,
		VK_F12,
		VK_NUMPAD0,
		VK_NUMPAD1,
		VK_NUMPAD2,
		VK_NUMPAD3,
		VK_NUMPAD4,
		VK_NUMPAD5,
		VK_NUMPAD6,
		VK_NUMPAD7,
		VK_NUMPAD8,
		VK_NUMPAD9,
		VK_DECIMAL,
		VK_DIVIDE,
		VK_MULTIPLY,
		VK_SUBTRACT,
		VK_ADD,
		//0,
		// 0,
		VK_LSHIFT,
		VK_LCONTROL,
		VK_LMENU,
		// 0,
		VK_RSHIFT,
		VK_RCONTROL,
		VK_RMENU,
		// 0,
		// 0,
	};
	constexpr int MaxNativeKeyValue = 255;
	static KeyCode nativeKey2ShitKeyMap[MaxNativeKeyValue];

#endif
	uint32_t MapKey(KeyCode keycode)
	{
		return shitKey2NativeKeyMap[static_cast<size_t>(keycode)];
	}
	KeyCode MapKey(uint32_t keycode)
	{
		static bool flag = true;
		if (flag)
		{
			for (int i = 0, len = sizeof shitKey2NativeKeyMap / sizeof shitKey2NativeKeyMap[0]; i < len; ++i)
				nativeKey2ShitKeyMap[shitKey2NativeKeyMap[i]] = static_cast<KeyCode>(i);
			flag = false;
		}
		return nativeKey2ShitKeyMap[static_cast<size_t>(keycode)];
	}
	EventModifierBits MapKeyModifier(uint32_t mods)
	{
		EventModifierBits modifiers{};

		//if (mods & MK_AL)
		//{
		//	modifiers |= SHIT_MOD_ALT;
		//}

		if (mods & MK_CONTROL)
		{
			modifiers |= EventModifierBits::CTRL;
		}

		if (mods & MK_SHIFT)
		{
			modifiers |= EventModifierBits::SHIFT;
		}
		if (mods & MK_LBUTTON)
		{
			modifiers |= EventModifierBits::BUTTONL;
		}
		if (mods & MK_RBUTTON)
		{
			modifiers |= EventModifierBits::BUTTONR;
		}
		if (mods & MK_MBUTTON)
		{
			modifiers |= EventModifierBits::BUTTONM;
		}
		if (mods & MK_XBUTTON1)
		{
			modifiers |= EventModifierBits::BUTTONX1;
		}
		if (mods & MK_XBUTTON2)
		{
			modifiers |= EventModifierBits::BUTTONX2;
		}

		return modifiers;
	}
}