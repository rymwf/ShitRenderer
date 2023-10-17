#pragma once
#include "ShitWindow.h"
namespace Shit
{
class X11Window : public ShitWindow
{
    Window mWindow;
    Display *mDisplay;
    XWindowChanges mWindowChange;

public:
    void Create(int width = SHIT_DEFAULT_WINDOW_WIDTH, int height = SHIT_DEFAULT_WINDOW_HEIGHT) override;
    void SetSize(int width, int height) override;

    void SetTitle(const char *title) override;
    void SetPos(int x, int y) override;
    void Close() override;
    void PollEvent() override;
};

} // namespace Shit