/**
 * @file ShitSurface.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitSwapchain.hpp"

namespace Shit {
class Surface {
protected:
    std::unique_ptr<Swapchain> mSwapchain;

    std::optional<QueueFamily> mPresentQueueFamily;

    PhysicalDevice mPhysicalDevice;

public:
    Surface(PhysicalDevice physicalDevice) : mPhysicalDevice(physicalDevice) {}

    virtual ~Surface() {}

    constexpr PhysicalDevice GetPhysicalDevice() const { return mPhysicalDevice; }

    constexpr std::optional<QueueFamily> GetPresentQueueFamily() { return mPresentQueueFamily; }

    virtual Swapchain *Create(const SwapchainCreateInfo &createInfo) = 0;

    void DestroySwapchain() { mSwapchain.reset(); }

    Swapchain *GetSwapchain() const {
        if (mSwapchain) return mSwapchain.get();
        ST_LOG("swapchain is not created yet")
        return nullptr;
    }
    virtual void GetPresentModes(std::vector<PresentMode> &presentModes) const = 0;

    virtual void GetPixelFormats(std::vector<SurfacePixelFormat> &formats) const = 0;

    virtual void GetCapabilities(SurfaceCapabilities &caps) const = 0;
};
}  // namespace Shit
