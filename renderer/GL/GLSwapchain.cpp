/**
 * @file GLSwapchain.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLSwapchain.hpp"

#include "GLDevice.hpp"
#include "GLFence.hpp"
#include "GLFramebuffer.hpp"
#include "GLImage.hpp"
#include "GLRenderPass.hpp"
#include "GLRenderSystem.hpp"
#include "GLSemaphore.hpp"
#include "GLState.hpp"
#include "GLSurface.hpp"

namespace Shit {
GLSwapchain::GLSwapchain(GLSurface *pSurface, const SwapchainCreateInfo &createInfo) : Swapchain(pSurface, createInfo) {
    mpStateManager = pSurface->GetStateManager();

#ifdef CLIP_ORIGIN_UPPER_LEFT
    mpStateManager->ClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
#else
    mpStateManager->ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif
    // cubemap seamless
    mpStateManager->EnableCapability(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#ifndef NDEBUG
    GL::listGLInfo();
#endif
    CreateImages(GetSwapchainImageCount());
}
GLSwapchain::~GLSwapchain() {
    // mpDevice->Destroy(mpRenderPass);
    // mpDevice->Destroy(mpFramebuffer);
    // for (auto &&e : mpImageViews)
    //	mpDevice->Destroy(e);
    glDeleteFramebuffers(1, &mReadFBO);
    mpStateManager->NotifyReleasedFramebuffer(mReadFBO);
}
void GLSwapchain::CreateImages(uint32_t count) {
    mImages.resize(count);
    mImageViews.resize(count);
    if (count == 1) {
        // default fbo
        mImages.emplace_back(new GLImage(mpStateManager, GLImageFlag::BACK_LEFT_FRAMEBUFFER));
        ST_LOG("swapchain image count should be greater than 1")
        return;
    }

    ImageCreateInfo imageCreateInfo{{},
                                    ImageType::TYPE_2D,
                                    mCreateInfo.format,
                                    {mCreateInfo.imageExtent.width, mCreateInfo.imageExtent.height, 1u},
                                    1,
                                    1,
                                    SampleCountFlagBits::BIT_1,
                                    ImageTiling::OPTIMAL,
                                    ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                    MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

    ImageViewCreateInfo imageViewCI{
        0, ImageViewType::TYPE_2D, mCreateInfo.format, {}, {ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

    glGenFramebuffers(1, &mReadFBO);
    mpStateManager->PushDrawFramebuffer(mReadFBO);

    std::vector<GLenum> bindpoints(count);
    for (uint32_t i = 0; i < count; ++i) {
        // auto pImage = static_cast<const GLImage *>(mImages.emplace_back());
        mImages[i] = new GLImage(mpStateManager, imageCreateInfo, nullptr);
        auto pImage = static_cast<const GLImage *>(mImages[i]);
        // create  image view
        imageViewCI.pImage = pImage;
        mImageViews[i] = new GLImageView(mpStateManager, imageViewCI);

        bindpoints[i] = GL_COLOR_ATTACHMENT0 + i;
        auto imageFlag = pImage->GetImageFlag();
        // bind fbo attachment
        if (imageFlag == GLImageFlag::TEXTURE)
            glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindpoints[i], pImage->GetHandle(), 0);
        else if (imageFlag == GLImageFlag::RENDERBUFFER)
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindpoints[i], GL_RENDERBUFFER, pImage->GetHandle());
    }
    mpStateManager->PopDrawFramebuffer();
    mpStateManager->BindReadFramebuffer(mReadFBO);
    glReadBuffer(bindpoints[0]);
}

/**
 * @brief TODO: handle not ready, timeout and other cases
 *
 * @param info
 * @param index
 * @return Result
 */
Result GLSwapchain::GetNextImage(const GetNextImageInfo &info, uint32_t &index) {
    if (mImages.size() > 1) {
        index = mAvailableImageIndex;
        uint32_t width, height;
        static_cast<GLSurface *>(mSurface)->GetFramebufferSize(width, height);
        if (width != mCreateInfo.imageExtent.width || height != mCreateInfo.imageExtent.height) {
            mCreateInfo.imageExtent = {width, height};
            return Result::SHIT_ERROR_OUT_OF_DATE;
        }
        if (info.pFence) static_cast<GLFence const *>(info.pFence)->Reset();
        if (info.pSemaphore) static_cast<GLSemaphore const *>(info.pSemaphore)->Reset();
    }
    return Result::SUCCESS;
}
void GLSwapchain::Present() const {
    static_cast<GLSurface *>(mSurface)->MakeCurrent();

    if (mImages.size() > 1) {
        auto index = mAvailableImageIndex - 1;
        // auto index = mAvailableImageIndex;
        index %= mImages.size();
        // already render to a fbo, blit to default framebuffer
        mpStateManager->BindReadFramebuffer(mReadFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
        mpStateManager->BindDrawFramebuffer(0);  // default fbo drawbuffer is leftback buffer
        // glDrawBuffer(GL_BACK_LEFT);

        glBlitFramebuffer(0, 0, mCreateInfo.imageExtent.width, mCreateInfo.imageExtent.height, 0, 0,
                          mCreateInfo.imageExtent.width, mCreateInfo.imageExtent.height, GL_COLOR_BUFFER_BIT,
                          GL_NEAREST);
        mAvailableImageIndex += 1;
        mAvailableImageIndex %= mImages.size();
    }
    static_cast<GLSurface *>(mSurface)->Swapbuffer();
}
void GLSwapchain::Resize(uint32_t width, uint32_t height) {
    static_cast<GLSurface *>(mSurface)->MakeCurrent();
    mCreateInfo.imageExtent = {width, height};
    DestroyImages();
    glDeleteFramebuffers(1, &mReadFBO);
    mpStateManager->NotifyReleasedFramebuffer(mReadFBO);
    CreateImages(GetSwapchainImageCount());
    g_RenderSystem->GetMainSurface()->MakeCurrent();
}
}  // namespace Shit
