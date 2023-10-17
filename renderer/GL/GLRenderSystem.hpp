/**
 * @file GLRenderSystem.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitRenderSystem.hpp>

#include "GLPrerequisites.hpp"

namespace Shit {
class GLRenderSystem final : public RenderSystem {
    std::unique_ptr<GLSurface> mMainSurface;

    friend class GLSurface;

    void SetGLVersion(Shit::RendererVersion rendererVersion);

public:
    GLRenderSystem(const RenderSystemCreateInfo &createInfo);

    ~GLRenderSystem() { mDevices.clear(); }

    void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

    Device *CreateDevice(const DeviceCreateInfo &createInfo) override;

    // main render context
    GLSurface *GetMainSurface();

#ifdef _WIN32
    Surface *CreateSurface(const SurfaceCreateInfoWin32 &createInfo) override;
#endif
};
}  // namespace Shit