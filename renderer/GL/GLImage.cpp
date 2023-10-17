/**
 * @file GLImage.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLImage.hpp"

#include "GLSampler.hpp"
#include "GLState.hpp"
namespace Shit {
GLImage::~GLImage() {
    if (mImageFlag == GLImageFlag::TEXTURE) {
        mpStateManager->NotifyReleasedTexture(Map(mCreateInfo.imageType, mCreateInfo.samples), mHandle);
        glDeleteTextures(1, &mHandle);
    } else if (mImageFlag == GLImageFlag::RENDERBUFFER) {
        mpStateManager->NotifyReleasedRenderbuffer(mHandle);
        glDeleteRenderbuffers(1, &mHandle);
    }
    mDestroySignal(this);
}
GLImage::GLImage(GLStateManager *pStateManager, GLImageFlag imageFlag)
    : mpStateManager(pStateManager), mImageFlag(imageFlag) {}
void GLImage::AddDestroySignalListener(Slot<void(GLImage const *)> const &slot) const { mDestroySignal.Connect(slot); }
void GLImage::RemoveDestroySignalListener(Slot<void(GLImage const *)> const &slot) const {
    mDestroySignal.Disconnect(slot);
}
void GLImage::Init() {
    if (mCreateInfo.mipLevels == 0)
        mCreateInfo.mipLevels = static_cast<uint32_t>(
            std::floor(std::log2((std::max)((std::max)(mCreateInfo.extent.width, mCreateInfo.extent.height),
                                            mCreateInfo.extent.depth))) +
            1);
    uint32_t width = mCreateInfo.extent.width;
    uint32_t height = mCreateInfo.extent.height;
    uint32_t depth = mCreateInfo.extent.depth;
    uint32_t formatSize = GetFormatSize(mCreateInfo.format);
    for (uint32_t i = 0; i < mCreateInfo.mipLevels; ++i, width = (std::max)(width / 2, 1u),
                  height = (std::max)(height / 2, 1u), depth = (std::max)(depth / 2, 1u)) {
        mMemorySize += width * height * depth * formatSize * mCreateInfo.arrayLayers;
    }

    if (!bool(mCreateInfo.flags & ImageCreateFlagBits::MUTABLE_FORMAT_BIT) &&
        bool(mCreateInfo.usageFlags &
             (ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT)) &&
        !bool(mCreateInfo.usageFlags & (ImageUsageFlagBits::INPUT_ATTACHMENT_BIT | ImageUsageFlagBits::SAMPLED_BIT)) &&
        mCreateInfo.mipLevels == 1 && mCreateInfo.extent.depth == 1 && mCreateInfo.arrayLayers == 1) {
        mImageFlag = GLImageFlag::RENDERBUFFER;
        glGenRenderbuffers(1, &mHandle);
        mpStateManager->PushRenderbuffer(mHandle);
        if (mCreateInfo.samples > SampleCountFlagBits::BIT_1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, static_cast<GLsizei>(mCreateInfo.samples),
                                             MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                                             mCreateInfo.extent.height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                                  mCreateInfo.extent.height);
        }
        mpStateManager->PopRenderbuffer();
    } else {
        glGenTextures(1, &mHandle);
        GLenum target = Map(mCreateInfo.imageType, mCreateInfo.samples);
        mpStateManager->PushTextureUnit(0, target, mHandle);
        // if (static_cast<bool>(mCreateInfo.flags &
        // ImageCreateFlagBits::MUTABLE_FORMAT_BIT))
        if constexpr (0) {
            if (mCreateInfo.samples > SampleCountFlagBits::BIT_1) {
                glTexImage3DMultisample(target, static_cast<GLsizei>(mCreateInfo.samples),
                                        MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                                        mCreateInfo.extent.height,
                                        (std::max)(mCreateInfo.extent.depth, mCreateInfo.arrayLayers), GL_FALSE);
            } else {
                switch (mCreateInfo.imageType) {
                    case ImageType::TYPE_1D:
                        glTexImage2D(target, 0, MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                                     (std::max)(mCreateInfo.arrayLayers, mCreateInfo.extent.height), 0,
                                     MapPixelFormat(mCreateInfo.format), Map(GetFormatDataType(mCreateInfo.format)),
                                     nullptr);
                        break;
                    case ImageType::TYPE_2D:
                    case ImageType::TYPE_3D:
                        glTexImage3D(
                            target, 0, MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                            mCreateInfo.extent.height, (std::max)(mCreateInfo.arrayLayers, mCreateInfo.extent.depth), 0,
                            MapPixelFormat(mCreateInfo.format), Map(GetFormatDataType(mCreateInfo.format)), nullptr);
                        break;
                }
            }
        } else {
            if (mCreateInfo.samples > SampleCountFlagBits::BIT_1) {
                glTexStorage3DMultisample(target, static_cast<GLsizei>(mCreateInfo.samples),
                                          MapInternalFormat(mCreateInfo.format), mCreateInfo.extent.width,
                                          mCreateInfo.extent.height,
                                          (std::max)(mCreateInfo.arrayLayers, mCreateInfo.extent.depth), GL_FALSE);
            } else {
                switch (mCreateInfo.imageType) {
                    case ImageType::TYPE_1D:
                        glTexStorage2D(target, mCreateInfo.mipLevels, MapInternalFormat(mCreateInfo.format),
                                       mCreateInfo.extent.width,
                                       (std::max)(mCreateInfo.arrayLayers, mCreateInfo.extent.height));
                        break;
                    case ImageType::TYPE_2D:
                    case ImageType::TYPE_3D:
                        glTexStorage3D(target, mCreateInfo.mipLevels, MapInternalFormat(mCreateInfo.format),
                                       mCreateInfo.extent.width, mCreateInfo.extent.height,
                                       (std::max)(mCreateInfo.arrayLayers, mCreateInfo.extent.depth));
                        break;
                }
            }
        }
        mpStateManager->PopTextureUnit();
    }
}
GLImage::GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, const void *pData)
    : Image(createInfo), mpStateManager(pStateManager) {
    Init();
    if (pData) {
        UpdateSubData({}, {}, 0,
                      {{},
                       {mCreateInfo.extent.width,
                        mCreateInfo.imageType == ImageType::TYPE_1D
                            ? (std::max)(mCreateInfo.extent.height, mCreateInfo.arrayLayers)
                            : mCreateInfo.extent.height,
                        mCreateInfo.imageType == ImageType::TYPE_2D
                            ? (std::max)(mCreateInfo.extent.depth, mCreateInfo.arrayLayers)
                            : mCreateInfo.extent.depth}},
                      {}, pData);
    }
}
GLImage::GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, int val)
    : Image(createInfo), mpStateManager(pStateManager) {
    Init();
    UpdateSubData(
        {}, {}, 0,
        {{},
         {mCreateInfo.extent.width,
          mCreateInfo.imageType == ImageType::TYPE_1D ? (std::max)(mCreateInfo.extent.height, mCreateInfo.arrayLayers)
                                                      : mCreateInfo.extent.height,
          mCreateInfo.imageType == ImageType::TYPE_2D ? (std::max)(mCreateInfo.extent.depth, mCreateInfo.arrayLayers)
                                                      : mCreateInfo.extent.depth}},
        {}, val);
}
void GLImage::UpdateSubData(ImageLayout, ImageLayout, uint32_t mipLevel, const Rect3D &rect, ImageAspectFlagBits,
                            const void *pData) const {
    if (mImageFlag == GLImageFlag::TEXTURE) {
        GLenum target = Map(mCreateInfo.imageType, mCreateInfo.samples);
        mpStateManager->PushTextureUnit(0, target, mHandle);
        switch (mCreateInfo.imageType) {
            case ImageType::TYPE_1D:
                glTexSubImage2D(target, mipLevel, rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height,
                                MapPixelFormat(mCreateInfo.format), Map(GetFormatDataType(mCreateInfo.format)), pData);
                break;
            case ImageType::TYPE_2D:
            case ImageType::TYPE_3D:
                glTexSubImage3D(target, mipLevel, rect.offset.x, rect.offset.y, rect.offset.z, rect.extent.width,
                                rect.extent.height, rect.extent.depth, MapPixelFormat(mCreateInfo.format),
                                Map(GetFormatDataType(mCreateInfo.format)), pData);
                break;
        }
        mpStateManager->PopTextureUnit();
    } else {
        ST_LOG(" current image isn't texture, imageFlag ", static_cast<int>(mImageFlag))
    }
}
void GLImage::UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                            ImageAspectFlagBits aspectMask, int val) const {
    uint64_t offset = rect.offset.x * rect.offset.y * rect.offset.z * GetFormatSize(mCreateInfo.format);
    uint64_t size = rect.extent.width * rect.extent.height * rect.extent.depth * GetFormatSize(mCreateInfo.format);
    std::vector<int> data((size - offset) / 4, val);
    UpdateSubData(initialLayout, finalLayout, mipLevel, rect, aspectMask, data.data());
}
void GLImage::MapMemory(uint64_t offset, [[maybe_unused]] uint64_t size, void **ppData) const {
    if (mImageFlag == GLImageFlag::TEXTURE) {
        mpStateManager->BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        auto target = Map(mCreateInfo.imageType, mCreateInfo.samples);
        mpStateManager->BindTextureUnit(0, target, mHandle);
        mTempData.resize(mCreateInfo.extent.width * mCreateInfo.extent.height * GetFormatSize(mCreateInfo.format));
        glGetTexImage(target, 0, MapBaseFormat(mCreateInfo.format), Map(GetFormatDataType(mCreateInfo.format)),
                      mTempData.data());
        *ppData = mTempData.data() + offset;
    } else {
        ST_LOG(" current image isn't texture, imageFlag ", static_cast<int>(mImageFlag))
    }
}
void GLImage::UnMapMemory() const { UpdateSubData({}, {}, 0, Rect3D{{}, mCreateInfo.extent}, {}, mTempData.data()); }
void GLImage::FlushMappedMemoryRange([[maybe_unused]] uint64_t offset, [[maybe_unused]] uint64_t size) const {}
void GLImage::InvalidateMappedMemoryRange([[maybe_unused]] uint64_t offset, [[maybe_unused]] uint64_t size) const {
    ST_LOG("GL InvalidateMappedMemoryRange not implemented yet")
}
void GLImage::InvalidateTexSubImage(int level, int xoffset, int yoffset, int zoffset, uint32_t width, uint32_t height,
                                    uint32_t depth) const {
    if (mImageFlag == GLImageFlag::TEXTURE)
        glInvalidateTexSubImage(mHandle, level, xoffset, yoffset, zoffset, width, height, depth);
    else {
        ST_LOG(" current image isn't texture, imageFlag ", static_cast<int>(mImageFlag))
    }
}
void GLImage::GenerateMipmap(Filter, ImageLayout, ImageLayout) const {
    if (mImageFlag == GLImageFlag::TEXTURE) {
        // TODO: generate mipmap using different filter
        auto target = Map(mCreateInfo.imageType, mCreateInfo.samples);
        mpStateManager->PushTextureUnit(0, target, mHandle);
        glGenerateMipmap(target);
        mpStateManager->PopTextureUnit();
    } else {
        ST_LOG(" current image isn't texture, imageFlag ", static_cast<int>(mImageFlag))
    }
}
void GLImage::GetImageSubresourceLayout([[maybe_unused]] const ImageSubresource &subresouce,
                                        [[maybe_unused]] SubresourceLayout &subresourceLayout) const {
    //
    ST_LOG("opengl GetImageSubresourceLayout not implemented yet");
    // if (mImageFlag == GLImageFlag::TEXTURE)
    //{
    //	auto target = Map(mCreateInfo.imageType, mCreateInfo.samples);
    //	mpStateManager->PushTextureUnit(0, target, mHandle);
    //	mpStateManager->PopTextureUnit();
    // }
}
//===================================================================

GLImageView::GLImageView(GLStateManager *pStateManger, const ImageViewCreateInfo &createInfo)
    : ImageView(createInfo), mpStateManger(pStateManger) {
    auto pImage = static_cast<GLImage const *>(createInfo.pImage);
    auto imageFlag = pImage->GetImageFlag();
    if (imageFlag == GLImageFlag::TEXTURE) {
        auto target = Map(createInfo.viewType, pImage->GetCreateInfoPtr()->samples);
        auto internalFormat = MapInternalFormat(createInfo.format);
        glGenTextures(1, &mHandle);
        if (GLEW_VERSION_4_3) {
            glTextureView(mHandle, target, pImage->GetHandle(), internalFormat,
                          createInfo.subresourceRange.baseMipLevel, createInfo.subresourceRange.levelCount,
                          createInfo.subresourceRange.baseArrayLayer, createInfo.subresourceRange.layerCount);
        } else if (glIsExtensionSupported("GL_EXT_texture_view")) {
            glTextureViewEXT(mHandle, target, pImage->GetHandle(), internalFormat,
                             createInfo.subresourceRange.baseMipLevel, createInfo.subresourceRange.levelCount,
                             createInfo.subresourceRange.baseArrayLayer, createInfo.subresourceRange.layerCount);
        } else {
            ST_THROW("texture view not supported");
        }
        mpStateManger->PushTextureUnit(0, target, mHandle);
        if (mCreateInfo.subresourceRange.aspectMask == Shit::ImageAspectFlagBits::DEPTH_BIT)
            glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
        else if (mCreateInfo.subresourceRange.aspectMask == Shit::ImageAspectFlagBits::STENCIL_BIT)
            glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);

        GLint swizzles[4]{
            GL_RED,
            GL_GREEN,
            GL_BLUE,
            GL_ALPHA,
        };
        if (createInfo.components.r != ComponentSwizzle::IDENTITY) swizzles[0] = Map(createInfo.components.r);
        if (createInfo.components.g != ComponentSwizzle::IDENTITY) swizzles[1] = Map(createInfo.components.g);
        if (createInfo.components.b != ComponentSwizzle::IDENTITY) swizzles[2] = Map(createInfo.components.b);
        if (createInfo.components.a != ComponentSwizzle::IDENTITY) swizzles[3] = Map(createInfo.components.a);
        glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzles);
        mpStateManger->PopTextureUnit();
    } else
        mHandle = pImage->GetHandle();
}
void GLImageView::SetSampler(Sampler const *pSampler) {
    if (mpSampler == pSampler) return;
    mpSampler = pSampler;
    GLenum target = Map(mCreateInfo.viewType, mCreateInfo.pImage->GetCreateInfoPtr()->samples);
    mpStateManger->PushTextureUnit(0, target, mHandle);

    auto pSamplerCreateInfo = pSampler->GetCreateInfoPtr();

    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, Map(pSamplerCreateInfo->magFilter));
    if (pSamplerCreateInfo->mipmapMode == SamplerMipmapMode::LINEAR) {
        if (pSamplerCreateInfo->minFilter == Filter::LINEAR)
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    } else {
        if (pSamplerCreateInfo->minFilter == Filter::LINEAR)
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
        else
            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    }
    glTexParameterf(target, GL_TEXTURE_LOD_BIAS, pSamplerCreateInfo->mipLodBias);
    glTexParameterf(target, GL_TEXTURE_MIN_LOD, pSamplerCreateInfo->minLod);
    glTexParameterf(target, GL_TEXTURE_MAX_LOD, pSamplerCreateInfo->maxLod);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, Map(pSamplerCreateInfo->wrapModeU));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, Map(pSamplerCreateInfo->wrapModeV));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, Map(pSamplerCreateInfo->wrapModeW));
    if (pSamplerCreateInfo->compareEnable) {
        glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, Map(pSamplerCreateInfo->compareOp));
    }

    auto borderColor = Map(pSamplerCreateInfo->borderColor);
    std::visit(
        [target](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::array<float, 4>>)
                glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, arg.data());
            else if constexpr (std::is_same_v<T, std::array<int32_t, 4>>)
                glTexParameterIiv(target, GL_TEXTURE_BORDER_COLOR, arg.data());
        },
        borderColor);
    mpStateManger->PopTextureUnit();
}

GLImageView::~GLImageView() {
    if (static_cast<GLImage const *>(mCreateInfo.pImage)->GetImageFlag() == GLImageFlag::TEXTURE) {
        GLenum target = Map(mCreateInfo.viewType, mCreateInfo.pImage->GetCreateInfoPtr()->samples);
        mpStateManger->NotifyReleasedTexture(target, mHandle);
        glDeleteTextures(1, &mHandle);
    }
}
}  // namespace Shit
