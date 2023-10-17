/**
 * @file ShitSwapchain.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitImage.hpp"
#include "ShitRendererPrerequisites.hpp"

namespace Shit {
class Swapchain {
protected:
    SwapchainCreateInfo mCreateInfo;
    Surface *mSurface;

    std::vector<Image *> mImages;
    std::vector<ImageView *> mImageViews;

    void DestroyImages() {
        for (auto e : mImageViews) delete e;
        for (auto e : mImages) delete e;
    }

public:
    Swapchain(Surface *surface, const SwapchainCreateInfo &createInfo) : mCreateInfo(createInfo), mSurface(surface) {}
    virtual ~Swapchain() {}
    ST_CONSTEXPR const SwapchainCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    constexpr Surface *GetSurface() const { return mSurface; }

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    void GetImages(Image **images) const { memcpy(images, mImages.data(), sizeof(ptrdiff_t) * mImages.size()); }

    Image *GetImageByIndex(uint32_t index) const { return mImages.at(index); }
    ImageView *GetImageViewByIndex(uint32_t index) const { return mImageViews.at(index); }

    ST_CONSTEXPR uint32_t GetImageCount() const { return static_cast<uint32_t>(mImages.size()); }
    virtual Result GetNextImage(const GetNextImageInfo &info, uint32_t &index) = 0;
};
}  // namespace Shit
