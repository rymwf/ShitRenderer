/**
 * @file VKImage.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKImage.hpp"

#include "VKBuffer.hpp"
#include "VKCommandBuffer.hpp"
#include "VKDevice.hpp"
namespace Shit {
VKImage::~VKImage() {
    if (!mIsSwapchainImage) {
        vkDestroyImage(static_cast<VKDevice *>(mpDevice)->GetHandle(), mHandle, nullptr);
        vkFreeMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory, nullptr);
    }
}
ImageLayout VKImage::Init() {
    if (mCreateInfo.mipLevels == 0)
        mCreateInfo.mipLevels =
            static_cast<uint32_t>((std::floor)((std::log2)((std::max)(
                                      (std::max)((float)mCreateInfo.extent.width, (float)mCreateInfo.extent.height),
                                      (float)mCreateInfo.extent.depth))) +
                                  1.f);
    auto device = mpDevice->GetHandle();

#ifndef NDEBUG
    VkImageFormatProperties imageFormatProperties;
    // try
    //{
    {
        auto res = vkGetPhysicalDeviceImageFormatProperties(
            static_cast<VKDevice *>(mpDevice)->GetPhysicalDevice(), Map(mCreateInfo.format), Map(mCreateInfo.imageType),
            Map(mCreateInfo.tiling), Map(mCreateInfo.usageFlags), Map(mCreateInfo.flags), &imageFormatProperties);
        if (res != VK_SUCCESS) {
            ST_LOG("image format do not supported:", Map(mCreateInfo.format), ", tilling:", Map(mCreateInfo.tiling));
            exit(-1);
        }
    }
#endif

    ImageLayout tempLayout =
        mCreateInfo.initialLayout == ImageLayout::PREINITIALIZED ? ImageLayout::PREINITIALIZED : ImageLayout::UNDEFINED;
    mCreateInfo.usageFlags |= ImageUsageFlagBits::TRANSFER_SRC_BIT | ImageUsageFlagBits::TRANSFER_DST_BIT;

    VkImageCreateInfo imageCreateInfo{
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        nullptr,
        Map(mCreateInfo.flags),
        Map(mCreateInfo.imageType),
        Map(mCreateInfo.format),
        VkExtent3D{mCreateInfo.extent.width, mCreateInfo.extent.height, mCreateInfo.extent.depth},
        mCreateInfo.mipLevels,
        mCreateInfo.arrayLayers,
        static_cast<VkSampleCountFlagBits>(mCreateInfo.samples),
        Map(mCreateInfo.tiling),
        Map(mCreateInfo.usageFlags),
        VK_SHARING_MODE_EXCLUSIVE,
        0,
        nullptr,
        Map(tempLayout)};

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &mHandle) != VK_SUCCESS) ST_THROW("failed to create image");

    VkMemoryRequirements imageMemoryRequireMents;
    vkGetImageMemoryRequirements(device, mHandle, &imageMemoryRequireMents);

    mMemorySize = imageMemoryRequireMents.size;

    // ST_LOG_VAR(imageMemoryRequireMents.size);
    // ST_LOG_VAR(imageMemoryRequireMents.alignment);
    // ST_LOG_VAR(imageMemoryRequireMents.memoryTypeBits);

    auto memoryTypeIndex =
        VK::findMemoryTypeIndex(static_cast<VKDevice *>(mpDevice)->GetPhysicalDevice(),
                                imageMemoryRequireMents.memoryTypeBits, Map(mCreateInfo.memoryPropertyFlags));

    mMemory = VK::allocateMemory(device, imageMemoryRequireMents.size, memoryTypeIndex);

    vkBindImageMemory(device, mHandle, mMemory, 0);
    return tempLayout;
}
VKImage::VKImage(VKDevice *pDevice, const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask,
                 const void *pData)
    : Image(createInfo), VKDeviceObject(pDevice) {
    if (pData && (mCreateInfo.initialLayout == ImageLayout::UNDEFINED ||
                  mCreateInfo.initialLayout == ImageLayout::PREINITIALIZED)) {
        ST_LOG(
            "when image data is not null, the initial layout cannot be undefined "
            "or preinitialized");
        exit(-1);
    }
    auto tempLayout = Init();

    if (pData) {
        UpdateSubData(tempLayout, mCreateInfo.initialLayout, 0,
                      {{},
                       {mCreateInfo.extent.width, mCreateInfo.extent.height,
                        (std::max)(mCreateInfo.extent.depth, mCreateInfo.arrayLayers)}},
                      aspectMask, pData);
    } else if (mCreateInfo.initialLayout != ImageLayout::UNDEFINED &&
               mCreateInfo.initialLayout != ImageLayout::PREINITIALIZED) {
        mpDevice->ExecuteOneTimeCommand([&](CommandBuffer *cmdBuffer) {
            auto formatAttribute = GetFormatAttribute(mCreateInfo.format);
            ImageAspectFlagBits imageAspect;
            if (formatAttribute.baseFormat == BaseFormat::DEPTH)
                imageAspect = ImageAspectFlagBits::DEPTH_BIT;
            else if (formatAttribute.baseFormat == BaseFormat::STENCIL)
                imageAspect = ImageAspectFlagBits::STENCIL_BIT;
            else if (formatAttribute.baseFormat == BaseFormat::DEPTH_STENCIL)
                imageAspect = ImageAspectFlagBits::DEPTH_BIT | ImageAspectFlagBits::STENCIL_BIT;
            else
                imageAspect = ImageAspectFlagBits::COLOR_BIT;

            ImageMemoryBarrier barrier{{},
                                       AccessFlagBits::SHADER_READ_BIT,
                                       tempLayout,
                                       mCreateInfo.initialLayout,
                                       ST_QUEUE_FAMILY_IGNORED,
                                       ST_QUEUE_FAMILY_IGNORED,
                                       this,
                                       {imageAspect, 0, mCreateInfo.mipLevels, 0, mCreateInfo.arrayLayers}};

            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{PipelineStageFlagBits::TRANSFER_BIT,
                                                                 PipelineStageFlagBits::ALL_COMMANDS_BIT,
                                                                 {},
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 1,
                                                                 &barrier});
        });
    }
}
VKImage::VKImage(VKDevice *pDevice, const ImageCreateInfo &createInfo, ImageAspectFlagBits aspectMask, int val)
    : Image(createInfo), VKDeviceObject(pDevice) {
    auto tempLayout = Init();
    UpdateSubData(tempLayout, mCreateInfo.initialLayout, 0,
                  {{},
                   {mCreateInfo.extent.width, mCreateInfo.extent.height,
                    (std::max)(mCreateInfo.extent.depth, mCreateInfo.arrayLayers)}},
                  aspectMask, val);
}
void VKImage::CopyBufferToImage(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel,
                                const Rect3D &rect, ImageAspectFlagBits aspectMask, VKBuffer *stagingbuffer) const {
    static_cast<VKDevice *>(mpDevice)->ExecuteOneTimeCommand([&](CommandBuffer *cmdBuffer) {
        uint32_t baseLayer = 0, layerCount = 1, depth = 1;
        int32_t offsetz = 0;

        if (mCreateInfo.imageType != Shit::ImageType::TYPE_3D) {
            baseLayer = static_cast<uint32_t>(rect.offset.z);
            layerCount = rect.extent.depth;
        } else {
            offsetz = rect.offset.z;
            depth = rect.extent.depth;
        }

        ImageMemoryBarrier barrier{{},
                                   AccessFlagBits::TRANSFER_WRITE_BIT,
                                   initialLayout,
                                   ImageLayout::TRANSFER_DST_OPTIMAL,
                                   ST_QUEUE_FAMILY_IGNORED,
                                   ST_QUEUE_FAMILY_IGNORED,
                                   static_cast<const Image *>(this),
                                   {aspectMask, 0, mCreateInfo.mipLevels, 0, mCreateInfo.arrayLayers}};

        // 1. transfer layout from undefined to transder destination
        cmdBuffer->PipelineBarrier(PipelineBarrierInfo{PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT,
                                                       PipelineStageFlagBits::TRANSFER_BIT,
                                                       {},
                                                       0,
                                                       0,
                                                       0,
                                                       0,
                                                       1,
                                                       &barrier});

        BufferImageCopy bufferImageCopy;

        bufferImageCopy = BufferImageCopy{
            0,                                              // buffer offset
            0,                                              // buffer row length
            0,                                              // buffer image height
            {aspectMask, mipLevel, baseLayer, layerCount},  // image subresource
            {rect.offset.x, rect.offset.y, offsetz},        // image offset
            {rect.extent.width, rect.extent.height, depth}  // image extent
        };

        cmdBuffer->CopyBufferToImage(CopyBufferToImageInfo{stagingbuffer, this, 1,
                                                           //(uint32_t)bufferImageCopies.size(),
                                                           &bufferImageCopy});

        barrier = ImageMemoryBarrier{AccessFlagBits::TRANSFER_WRITE_BIT,
                                     AccessFlagBits::MEMORY_READ_BIT,
                                     ImageLayout::TRANSFER_DST_OPTIMAL,
                                     finalLayout,
                                     ST_QUEUE_FAMILY_IGNORED,
                                     ST_QUEUE_FAMILY_IGNORED,
                                     static_cast<const Image *>(this),
                                     {aspectMask, 0, mCreateInfo.mipLevels, 0, mCreateInfo.arrayLayers}};
        cmdBuffer->PipelineBarrier(PipelineBarrierInfo{
            PipelineStageFlagBits::TRANSFER_BIT, PipelineStageFlagBits::TOP_OF_PIPE_BIT, {}, 0, 0, 0, 0, 1, &barrier});
    });
}
void VKImage::UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                            ImageAspectFlagBits aspectMask, const void *pData) const {
    uint64_t offset = rect.offset.x * rect.offset.y * rect.offset.z * GetFormatSize(mCreateInfo.format);
    uint64_t size =
        rect.extent.width * rect.extent.height * rect.extent.depth * GetFormatSize(mCreateInfo.format) - offset;
    if (bool(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT)) {
        void *data;
        vkMapMemory(mpDevice->GetHandle(), mMemory, offset, size, 0, &data);
        memcpy(data, pData, size);
        vkUnmapMemory(mpDevice->GetHandle(), mMemory);
    } else {
        BufferCreateInfo stagingBufferCreateInfo{
            {},
            size,
            BufferUsageFlagBits::TRANSFER_SRC_BIT,
            MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
        };

        VKBuffer stagingbuffer{mpDevice, mpDevice->GetPhysicalDevice(), stagingBufferCreateInfo};
        void *data;
        stagingbuffer.MapMemory(0, size, &data);
        memcpy(data, pData, static_cast<size_t>(size));
        stagingbuffer.UnMapMemory();

        CopyBufferToImage(initialLayout, finalLayout, mipLevel, rect, aspectMask, &stagingbuffer);
    }
}
void VKImage::UpdateSubData(ImageLayout initialLayout, ImageLayout finalLayout, uint32_t mipLevel, const Rect3D &rect,
                            ImageAspectFlagBits aspectMask, int val) const {
    uint64_t offset = rect.offset.x * rect.offset.y * rect.offset.z * GetFormatSize(mCreateInfo.format);
    uint64_t size =
        rect.extent.width * rect.extent.height * rect.extent.depth * GetFormatSize(mCreateInfo.format) - offset;
    if (bool(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT)) {
        void *data;
        vkMapMemory(mpDevice->GetHandle(), mMemory, offset, size, 0, &data);
        memset(data, val, (size_t)size);
        vkUnmapMemory(mpDevice->GetHandle(), mMemory);
    } else {
        BufferCreateInfo stagingBufferCreateInfo{
            {},
            size,
            BufferUsageFlagBits::TRANSFER_SRC_BIT,
            MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
        };

        VKBuffer stagingbuffer{mpDevice, mpDevice->GetPhysicalDevice(), stagingBufferCreateInfo};
        void *data;
        stagingbuffer.MapMemory(0, size, &data);
        memset(data, val, (size_t)size);
        stagingbuffer.UnMapMemory();

        CopyBufferToImage(initialLayout, finalLayout, mipLevel, rect, aspectMask, &stagingbuffer);
    }
}
void VKImage::GenerateMipmap(Filter filter, ImageLayout initialLayout, ImageLayout finalLayout) const {
    mpDevice->ExecuteOneTimeCommand([&](CommandBuffer *cmdBuffer) {
        cmdBuffer->GenerateMipmap({this, filter, initialLayout, finalLayout});
    });
}
void VKImage::MapMemory(uint64_t offset, uint64_t size, void **ppData) const {
    vkMapMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory, offset, size, 0, ppData);
}
void VKImage::UnMapMemory() const { vkUnmapMemory(static_cast<VKDevice *>(mpDevice)->GetHandle(), mMemory); }
void VKImage::FlushMappedMemoryRange(uint64_t offset, uint64_t size) const {
    auto range = VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, mMemory, offset, size};
    vkFlushMappedMemoryRanges(static_cast<VKDevice *>(mpDevice)->GetHandle(), 1, &range);
}
void VKImage::InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const {
    auto range = VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, mMemory, offset, size};
    vkInvalidateMappedMemoryRanges(static_cast<VKDevice *>(mpDevice)->GetHandle(), 1, &range);
}
void VKImage::InvalidateTexSubImage([[maybe_unused]] int level, [[maybe_unused]] int xoffset,
                                    [[maybe_unused]] int yoffset, [[maybe_unused]] int zoffset,
                                    [[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height,
                                    [[maybe_unused]] uint32_t depth) const {
    ST_LOG("vk InvalidateTexSubImage not implemented yet");
}

void VKImage::GetImageSubresourceLayout(const ImageSubresource &subresouce,
                                        SubresourceLayout &subresourceLayout) const {
    VkSubresourceLayout ret;
    vkGetImageSubresourceLayout(static_cast<VKDevice *>(mpDevice)->GetHandle(), mHandle,
                                reinterpret_cast<const VkImageSubresource *>(&subresouce), &ret);
    memcpy(&subresourceLayout, &ret, sizeof(ret));
}
//=======================================================================================

VKImageView::VKImageView(VKDevice *device, const ImageViewCreateInfo &createInfo)
    : ImageView(createInfo), VKDeviceObject(device) {
    VkImageViewCreateInfo imageViewCreateInfo{
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        nullptr,
        0,
        static_cast<const VKImage *>(mCreateInfo.pImage)->GetHandle(),
        Map(mCreateInfo.viewType),
        Map(mCreateInfo.format),
        {
            Map(mCreateInfo.components.r),
            Map(mCreateInfo.components.g),
            Map(mCreateInfo.components.b),
            Map(mCreateInfo.components.a),
        },
        *reinterpret_cast<const VkImageSubresourceRange *>(&mCreateInfo.subresourceRange)};
    CHECK_VK_RESULT(vkCreateImageView(mpDevice->GetHandle(), &imageViewCreateInfo, nullptr, &mHandle))
}
}  // namespace Shit
