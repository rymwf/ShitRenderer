/**
 * @file VKSurface.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSurface.hpp>

#include "VKPrerequisites.hpp"
#include "VKSwapchain.hpp"

namespace Shit {
class VKSurface : public Surface {
protected:
    VkSurfaceKHR mHandle;

    void GetPresentQueueFamily();

public:
    VKSurface(PhysicalDevice physicalDevice);

    ~VKSurface() override;
    constexpr VkSurfaceKHR GetHandle() const { return mHandle; }

    Swapchain *Create(const SwapchainCreateInfo &createInfo) override;

    void GetPresentModes(std::vector<PresentMode> &presentModes) const override;

    void GetPixelFormats(std::vector<SurfacePixelFormat> &formats) const override;

    void GetCapabilities(SurfaceCapabilities &caps) const override;
};

#ifdef _WIN32
class VKSurfaceWin32 final : public VKSurface {
    SurfaceCreateInfoWin32 mCreateInfoWin32;

public:
    VKSurfaceWin32(PhysicalDevice physicalDevice, SurfaceCreateInfoWin32 const &mCreateInfoWin32);
    ~VKSurfaceWin32() {}
};
#endif
}  // namespace Shit