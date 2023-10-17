/**
 * @file VKSurface.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "VKSurface.hpp"

#include "VKRenderSystem.hpp"
#ifdef _WIN32
#include "renderer/Platform/Windows/ShitPrerequisitesWin32.hpp"
#endif
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif

namespace Shit {
VKSurface::VKSurface(PhysicalDevice physicalDevice) : Surface(physicalDevice) {}
void VKSurface::GetPresentQueueFamily() {
    auto queueFamilyProperties = static_cast<VKRenderSystem *>(g_RenderSystem)->GetQueueFamilyProperties();
    auto index = VK::findQueueFamilyIndexPresent((VkPhysicalDevice)mPhysicalDevice,
                                                 (uint32_t)queueFamilyProperties.size(), mHandle);

    if (index.has_value())
        mPresentQueueFamily = std::optional<QueueFamily>{
            {Map(queueFamilyProperties[*index].queueFlags), *index, queueFamilyProperties[*index].queueCount}};
}
VKSurface::~VKSurface() {
    mSwapchain.reset();
    vkDestroySurfaceKHR(g_RenderSystem->GetInstance(), mHandle, nullptr);
}
Swapchain *VKSurface::Create(const SwapchainCreateInfo &createInfo) {
    mSwapchain = std::make_unique<VKSwapchain>(this, createInfo);
    return mSwapchain.get();
}
void VKSurface::GetPresentModes(std::vector<PresentMode> &presentModes) const {
    std::vector<VkPresentModeKHR> modes;
    VK::querySurfacePresentModes((VkPhysicalDevice)mPhysicalDevice, mHandle, modes);
    presentModes.reserve(modes.size());
    for (auto e : modes) {
        presentModes.emplace_back(Map(e));
    }

#ifndef NDEBUG
    static const char *presentModeNames[]{
        "IMMEDIATE", "MAILBOX", "FIFO", "FIFO_RELAXED", "SHARED_DEMAND_REFRESH", "SHARED_CONTINUOUS_REFRESH",
    };
    ST_LOG("supported present mode:");
    for (auto &&e : presentModes) ST_LOG(static_cast<size_t>(e), presentModeNames[static_cast<size_t>(e)]);
#endif
}
void VKSurface::GetPixelFormats(std::vector<SurfacePixelFormat> &formats) const {
    std::vector<VkSurfaceFormatKHR> surfaceFormats;
    VK::querySurfaceFormats((VkPhysicalDevice)mPhysicalDevice, mHandle, surfaceFormats);
    formats.reserve(surfaceFormats.size());
    for (auto e : surfaceFormats) {
        formats.emplace_back(Map(e.format), Map(e.colorSpace));
    }
}
void VKSurface::GetCapabilities(SurfaceCapabilities &caps) const {
    VkSurfaceCapabilitiesKHR vkcaps;
    CHECK_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR((VkPhysicalDevice)mPhysicalDevice, mHandle, &vkcaps))
    caps.minImageCount = vkcaps.minImageCount;
    caps.maxImageCount = vkcaps.maxImageCount;
    memcpy(&caps.currentExtent, &vkcaps.currentExtent, sizeof(Extent2D) * 3);
    // memcpy(&caps.minImageExtent, &vkcaps.minImageExtent,
    // sizeof(caps.minImageExtent)); memcpy(&caps.maxImageExtent,
    // &vkcaps.maxImageExtent, sizeof(caps.maxImageExtent));
    caps.maxImageArrayLayers = vkcaps.maxImageArrayLayers;
}

#ifdef _WIN32
VKSurfaceWin32::VKSurfaceWin32(PhysicalDevice physicalDevice, SurfaceCreateInfoWin32 const &mCreateInfoWin32)
    : VKSurface(physicalDevice) {
    HINSTANCE hinstance = (HINSTANCE)GetWindowLongPtr((HWND)mCreateInfoWin32.hwnd, GWLP_HINSTANCE);

    VkWin32SurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0, hinstance,
                                     (HWND)mCreateInfoWin32.hwnd};
    if (vkCreateWin32SurfaceKHR(g_RenderSystem->GetInstance(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create VK surface");

    GetPresentQueueFamily();
}
#endif
}  // namespace Shit