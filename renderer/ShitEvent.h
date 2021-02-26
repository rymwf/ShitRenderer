/**
 * @file ShitEvent.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#pragma once
#include "ShitRendererPrerequisites.h"

#define SHIT_INPUT_EVENT_CONSTRUCTOR_IMPL(class_, type_) \
	class_() : InputEvent(type_) {}

namespace Shit
{
	struct MouseMoveEvent
	{
		float xpos;
		float ypos;
	};
	struct MouseEnterEvent
	{
		bool entered;
	};
	struct MouseButtonEvent
	{
		MouseButton button;
		PressAction action;
	};
	struct MouseWheelEvent
	{
		int xoffset; //+-1
		int yoffset; //+-1
	};
	struct KeyEvent
	{
		KeyCode key;
		PressAction action;
	};
	struct CharEvent
	{
		uint32_t codepoint;
	};
	struct DropEvent
	{
		uint32_t count;
		char **paths;
	};
	struct CloseEvent
	{
		ShitWindow *pWindow;
	};
	struct ResizeEvent
	{
		uint32_t width;
		uint32_t height;
	};
	struct ContentScaleEvent
	{
		float xscale;
		float yscale;
	};
	struct PosEvent
	{
		int xpos;
		int ypos;
	};
	struct IconifyEvent
	{
		bool iconified;
	};
	struct MaximizeEvent
	{
		bool maximized;
	};
	struct FocusEvent
	{
		bool focused;
	};
	struct RefreshEvent
	{
	};

	struct Event
	{
		EventType type;
		EventModifierBits modifier;
		union
		{
			KeyEvent key;
			MouseMoveEvent mouseMove;
			MouseButtonEvent mouseButton;
			MouseEnterEvent mouseEnter;
			CharEvent charInput;
			MouseWheelEvent mouseWheel;
			DropEvent drop;

			//window event
			CloseEvent windowClose;
			ResizeEvent windowResize;
			ContentScaleEvent windowConstentScale;
			PosEvent windowPos;
			IconifyEvent windowIconify;
			MaximizeEvent windowMaximize;
			FocusEvent windowFocus;
			RefreshEvent windowRefresh;
		};
	};



	uint32_t MapKey(KeyCode keycode);
	KeyCode MapKey(uint32_t keycode);
	EventModifierBits MapKeyModifier(uint32_t mods);
};