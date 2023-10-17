#include "input.hpp"

Shit::Signal<void(int, int)> Input::framebufferResizeSignal;
Shit::Signal<void(int, int, int)> Input::mouseButtonSignal;
Shit::Signal<void(double, double)> Input::mouseMoveSignal;
Shit::Signal<void(double, double)> Input::scrollSignal;
Shit::Signal<void(int, int, int, int)> Input::keyboardSignal;

void Input::onFramebufferResize(int width, int height) { framebufferResizeSignal(width, height); }
void Input::onMouseButton(int button, int action, int mods) { mouseButtonSignal(button, action, mods); }
void Input::onMouseMove(double xpos, double ypos) { mouseMoveSignal(xpos, ypos); }
void Input::onScroll(double xoffset, double yoffset) { scrollSignal(xoffset, yoffset); }
void Input::onKeyboard(int key, int scancode, int action, int mods) { keyboardSignal(key, scancode, action, mods); }