/**
 * @file ShitImage.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit {
class Image {
protected:
    bool mIsSwapchainImage{};
    ImageCreateInfo mCreateInfo;
    Image(const ImageCreateInfo &createInfo) : mCreateInfo(createInfo) {}
    Image() = default;

    uint64_t mMemorySize{};

public:
    virtual ~Image() {}
    constexpr const ImageCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    /**
     * @brief this function will change layout of  all layers and miplevels
     *
     * @param mipLevel
     * @param initialLayout
     * @param finalLayout
     * @param rect when teture array ,use z as array count
     * @param aspectMask
     * @param pData
     */
    virtual void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel,
                               const Rect3D &rect, ImageAspectFlagBits aspectMask, const void *pData) const = 0;
    /**
     * @brief this function will change layout of  all layers and miplevels
     *
     * @param mipLevel
     * @param initialLayout
     * @param finalLayout
     * @param rect when teture array ,use z as array count
     * @param aspectMask
     * @param val
     */
    virtual void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel,
                               const Rect3D &rect, ImageAspectFlagBits aspectMask, int val) const = 0;

    virtual void MapMemory(uint64_t offset, uint64_t size, void **ppData) const = 0;
    virtual void UnMapMemory() const = 0;
    virtual void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const = 0;
    virtual void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const = 0;
    virtual void InvalidateTexSubImage(int level, int xoffset, int yoffset, int zoffset, uint32_t width,
                                       uint32_t height, uint32_t depth) const = 0;

    /**
     * @brief
     *
     * @param filter
     * @param initialLayout
     * @param finalLayout change layout of whole image
     */
    virtual void GenerateMipmap(Filter filter, ImageLayout initialLayout, ImageLayout finalLayout) const = 0;

    // virtual void TransformLayout([[maybe_unused]] uint32_t baseMiplevel,
    // uint32_t levelCount, [[maybe_unused]] ImageLayout dstImageLayout)
    //{
    // }
    virtual void GetImageSubresourceLayout(const ImageSubresource &subresouce,
                                           SubresourceLayout &subresourceLayout) const = 0;

    constexpr uint64_t GetMemorySize() const { return mMemorySize; }
};

class ImageView {
protected:
    ImageViewCreateInfo mCreateInfo;
    ImageView(const ImageViewCreateInfo &createInfo) : mCreateInfo(createInfo) {}

public:
    virtual ~ImageView() {}
    constexpr const ImageViewCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};

}  // namespace Shit
