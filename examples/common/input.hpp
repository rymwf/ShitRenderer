#pragma once
#include <variant>

#include "renderer/ShitRenderSystem.hpp"

struct Input {
    static Shit::Signal<void(int, int)> framebufferResizeSignal;
    static Shit::Signal<void(int, int, int)> mouseButtonSignal;
    static Shit::Signal<void(double, double)> mouseMoveSignal;
    static Shit::Signal<void(double, double)> scrollSignal;
    static Shit::Signal<void(int, int, int, int)> keyboardSignal;

    static void onFramebufferResize(int width, int height);
    static void onMouseButton(int button, int action, int mods);
    static void onMouseMove(double xpos, double ypos);
    static void onScroll(double xoffset, double yoffset);
    static void onKeyboard(int key, int scancode, int action, int mods);
};
