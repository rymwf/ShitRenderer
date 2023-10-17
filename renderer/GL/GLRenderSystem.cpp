/**
 * @file GLRenderSystem.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLRenderSystem.hpp"

#include "GLDevice.hpp"
#include "GLSurface.hpp"

namespace Shit {
GLRenderSystem *g_RenderSystem;

extern "C" [[nodiscard]] SHIT_API Shit::RenderSystem *ShitLoadRenderSystem(
    const Shit::RenderSystemCreateInfo &createInfo) {
    g_RenderSystem = new GLRenderSystem(createInfo);
    return g_RenderSystem;
}
extern "C" SHIT_API void ShitDeleteRenderSystem(const Shit::RenderSystem *pRenderSystem) { delete pRenderSystem; }

GLRenderSystem::GLRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo) {}
void GLRenderSystem::SetGLVersion(Shit::RendererVersion rendererVersion) { mCreateInfo.version = rendererVersion; }
void GLRenderSystem::EnumeratePhysicalDevice([[maybe_unused]] std::vector<PhysicalDevice> &physicalDevices) {
    ST_LOG("currently GL do not support select gpu");
}

Device *GLRenderSystem::CreateDevice(const DeviceCreateInfo &createInfo) {
    return mDevices.emplace_back(std::make_unique<GLDevice>(createInfo)).get();
}

GLSurface *GLRenderSystem::GetMainSurface() {
    if (!mMainSurface) {
        // create main surface
#ifdef ST_WIN32
        mMainSurface = std::make_unique<GLSurfaceWin32>(PhysicalDevice{}, nullptr, SurfaceCreateInfoWin32{});
#else
        ST_THROW("GetMainSurface not implemented yet")
#endif
    }
    return mMainSurface.get();
}

#ifdef _WIN32
Surface *GLRenderSystem::CreateSurface(const SurfaceCreateInfoWin32 &createInfo) {
    auto ret =
        mSurfaces.emplace_back(std::make_unique<GLSurfaceWin32>(PhysicalDevice{}, GetMainSurface(), createInfo)).get();
    mMainSurface->MakeCurrent();
    return ret;
}
#endif
}  // namespace Shit
