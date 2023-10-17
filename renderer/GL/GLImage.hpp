/**
 * @file GLImage.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitImage.hpp>
#include <renderer/ShitListener.hpp>

#include "GLPrerequisites.hpp"
namespace Shit {
/**
 * @brief GLImage can represent texture, renderbuffer and backbuffer
 * when is texture, everything works fine
 * when is renderbuffer or framebuffer, updatesubdata, map will not work
 *
 */
class GLImage final : public Image {
    void Init();

public:
    GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, const void *pData);

    GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, int val);

    // used as an alias of default framebuffer
    GLImage(GLStateManager *pStateManager, GLImageFlag imageFlag);

    ~GLImage() override;
    constexpr GLuint GetHandle() const { return mHandle; }

    /**
     * @brief if image is renderer buffer or multisample image or backbuffer, do
     * not use this method
     *
     * @param imageSubData
     */
    void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                       ImageAspectFlagBits aspectMask, const void *pData) const override;

    void UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                       ImageAspectFlagBits aspectMask, int val) const override;

    constexpr GLImageFlag GetImageFlag() const { return mImageFlag; }
    void MapMemory(uint64_t offset, uint64_t size, void **ppData) const override;
    void UnMapMemory() const override;
    void FlushMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    void InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const override;
    void InvalidateTexSubImage(int level, int xoffset, int yoffset, int zoffset, uint32_t width, uint32_t height,
                               uint32_t depth) const override;

    void GenerateMipmap(Filter filter, ImageLayout intialLayout, ImageLayout finalLayout) const override;

    void GetImageSubresourceLayout(const ImageSubresource &subresouce,
                                   SubresourceLayout &subresourceLayout) const override;

    void AddDestroySignalListener(Slot<void(GLImage const *)> const &slot) const;
    void RemoveDestroySignalListener(Slot<void(GLImage const *)> const &slot) const;

private:
    GLuint mHandle{};
    GLStateManager *mpStateManager;
    mutable std::vector<unsigned char> mTempData;
    GLImageFlag mImageFlag{GLImageFlag::TEXTURE};  // 0 stand for texture,1 renderer buffer, 2
                                                   // backbuffer

    mutable Signal<void(GLImage const *)> mDestroySignal;
};

class GLImageView final : public ImageView {
    GLuint mHandle{};
    GLStateManager *mpStateManger;
    Sampler const *mpSampler;

public:
    GLImageView(GLStateManager *pStateManger, const ImageViewCreateInfo &createInfo);
    ~GLImageView() override;
    constexpr GLuint GetHandle() const { return mHandle; }
    void SetSampler(Sampler const *pSampler);
};
}  // namespace Shit