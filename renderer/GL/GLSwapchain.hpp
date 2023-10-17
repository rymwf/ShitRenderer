/**
 * @file GLSwapchain.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitSwapchain.hpp>

#include "GLPrerequisites.hpp"

namespace Shit {

/**
 * @brief images count is in [8,16]
 *
 */
class GLSwapchain : public Swapchain {
    GLStateManager *mpStateManager;
    mutable uint32_t mAvailableImageIndex{};

    // std::function<void(const Event &)> mProcessWindowEventCallable;

    GLuint mReadFBO;  // attach image to a read fbo

protected:
    void CreateImages(uint32_t count);

    // void CreateRenderPass();

    /**
     * @brief Create a Framebuffer object used to blit
     *
     */
    // void CreateFramebuffer();

    constexpr uint32_t GetSwapchainImageCount() const {
        return mCreateInfo.minImageCount;
        // return (std::min)((std::max)(mCreateInfo.minImageCount, 2U), 8U);
    }

public:
    GLSwapchain(GLSurface *pSurface, const SwapchainCreateInfo &createInfo);

    ~GLSwapchain() override;

    Result GetNextImage(const GetNextImageInfo &info, uint32_t &index) override;

    void Resize(uint32_t width, uint32_t height) override;

    /**
     * @brief if swapchain use default framebuffer, then do nothing,
     *  otherwise blit current image to backleft framebuffer
     *
     */
    void Present() const;
};
}  // namespace Shit
