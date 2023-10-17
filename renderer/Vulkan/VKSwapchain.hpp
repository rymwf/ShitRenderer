/**
 * @file VKSwapchain.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSwapchain.hpp>

#include "VKPrerequisites.hpp"

namespace Shit {
class VKSwapchain final : public Swapchain {
    VkSwapchainKHR mHandle;
    VkPresentModeKHR mPresentMode{};
    VkSurfaceFormatKHR mSurfaceFormat;

    void CreateSwapchain();
    void CreateImageViews();

public:
    VKSwapchain(Surface *surface, const SwapchainCreateInfo &createInfo);

    ~VKSwapchain() override;

    constexpr VkSurfaceFormatKHR GetSurfaceFormat() const { return mSurfaceFormat; }
    constexpr VkSwapchainKHR GetHandle() const { return mHandle; }
    constexpr VkPresentModeKHR GetPresentMode() const { return mPresentMode; }

    Result GetNextImage(const GetNextImageInfo &info, uint32_t &index) override;

    void Resize(uint32_t width, uint32_t height) override;
};

}  // namespace Shit
