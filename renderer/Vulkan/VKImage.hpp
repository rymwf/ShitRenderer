/**
 * @file VKImage.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitImage.hpp>

#include "VKBuffer.hpp"
#include "VKDeviceObject.hpp"
namespace Shit {
class VKImage final : public Image, public VKDeviceObject {
    VkImage mHandle;
    VkDeviceMemory mMemory;

    void CopyBufferToImage(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                           ImageAspectFlagBits aspectMask, VKBuffer *stagingbuffer) const;

    ImageLayout Init();

public:
    VKImage(VKDevice *pDevice, const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, const void *pData);
    VKImage(VKDevice *pDevice, const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, int val);
    VKImage(VKDevice *pDevice, const ImageCreateInfo &createInfo, VkImage image, bool isSwapchainimage)
        : Image(createInfo), VKDeviceObject(pDevice), mHandle(image) {
        mIsSwapchainImage = isSwapchainimage;
    }

    ~VKImage() override;

    constexpr VkImage GetHandle() const { return mHandle; }
    constexpr VkDeviceMemory GetMemory() const { return mMemory; }

    void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                       ImageAspectFlagBits aspectMask, const void *pData) const override;

    void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                       ImageAspectFlagBits aspectMask, int val) const override;

    void MapMemory(uint64_t offset, uint64_t size, void **ppData) const override;
    void UnMapMemory() const override;
    void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    void InvalidateTexSubImage(int level, int xoffset, int yoffset, int zoffset, uint32_t width, uint32_t height,
                               uint32_t depth) const override;

    void GenerateMipmap(Filter filter, ImageLayout initialLayout, ImageLayout finalLayout) const override;

    void GetImageSubresourceLayout(const ImageSubresource &subresouce,
                                   SubresourceLayout &subresourceLayout) const override;

    // void TransformLayout(uint32_t baseMiplevel, uint32_t levelCount,
    // ImageLayout dstImageLayout) override;
};

class VKImageView final : public ImageView, public VKDeviceObject {
    VkImageView mHandle;

public:
    VKImageView(VKDevice *device, const ImageViewCreateInfo &createInfo);
    ~VKImageView() override { vkDestroyImageView(mpDevice->GetHandle(), mHandle, nullptr); }
    constexpr VkImageView GetHandle() const { return mHandle; }
};

}  // namespace Shit
