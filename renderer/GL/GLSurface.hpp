#pragma once
#include <renderer/ShitSurface.hpp>

#include "GLState.hpp"

#ifdef _WIN32
#include <gl/wglew.h>
#endif

namespace Shit {
class GLSurface : public Surface {
protected:
    std::unique_ptr<GLStateManager> mpStateManager{};
    GLSurface *mShareSurface{};
    bool mIsCurrent{};

    void InitGL();

public:
    GLSurface(PhysicalDevice physicalDevice, GLSurface *shareSurface);

    ~GLSurface() {}

    Swapchain *Create(const SwapchainCreateInfo &createInfo) override;

    GLStateManager *GetStateManager() const { return mpStateManager.get(); }

    void GetPixelFormats(std::vector<SurfacePixelFormat> &formats) const override;

    void GetPresentModes(std::vector<PresentMode> &presentModes) const override;

    void EnableDebugOutput(const void *userParam);

    GLSurface *GetShareSurface() const { return mShareSurface; }

    virtual void SetSwapInterval(int interval) = 0;

    virtual void MakeCurrent() const = 0;

    virtual void Swapbuffer() const = 0;

    // framebuffer
    virtual void GetFramebufferSize(uint32_t &width, uint32_t &height) const = 0;
};

#ifdef _WIN32
class GLSurfaceWin32 : public GLSurface {
    HDC mHdc;
    HWND mHwnd{};
    HGLRC mHRenderContext;

    /**
     * @brief create a false rendercontext and init wgl extensions
     *
     */
    void InitWglExtentions();

    void CreateBackgroundWindow();
    void CreateContext();

    void SetSwapInterval(int interval) override;

public:
    GLSurfaceWin32(PhysicalDevice physicalDevice, GLSurface *shareSurface, const SurfaceCreateInfoWin32 &createInfo);
    ~GLSurfaceWin32();

    void MakeCurrent() const override;

    constexpr HDC GetHDC() const { return mHdc; }
    constexpr HWND GetHWND() const { return mHwnd; }
    constexpr HGLRC GetRenderContext() const { return mHRenderContext; }

    void Swapbuffer() const override;

    void GetFramebufferSize(uint32_t &width, uint32_t &height) const override;

    void GetCapabilities(SurfaceCapabilities &caps) const override;
};
#endif
}  // namespace Shit