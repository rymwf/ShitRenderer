/**
 * @file VKFramebuffer.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKFramebuffer.hpp"

#include "VKImage.hpp"
#include "VKRenderPass.hpp"
namespace Shit {

VKFramebuffer::VKFramebuffer(VKDevice *device, const FramebufferCreateInfo &createInfo)
    : Framebuffer(createInfo), VKDeviceObject(device) {
    std::vector<VkImageView> attachments;
    VkFramebufferCreateFlags flags{};
    void *pNext = nullptr;
    VkFramebufferAttachmentsCreateInfo frameBufferCI{VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO};
    std::vector<VkFramebufferAttachmentImageInfo> attachmentImageInfos;
    std::vector<std::vector<VkFormat>> viewFormats;

    if (mCreateInfo.pAttachmentImageInfos) {
        flags = VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        pNext = &frameBufferCI;
        attachmentImageInfos.resize(mCreateInfo.attachmentCount);
        viewFormats.resize(mCreateInfo.attachmentCount);
        frameBufferCI.attachmentImageInfoCount = mCreateInfo.attachmentCount;
        frameBufferCI.pAttachmentImageInfos = attachmentImageInfos.data();
    }

    for (uint32_t i = 0; i < mCreateInfo.attachmentCount; ++i) {
        if (mCreateInfo.pAttachments)
            attachments.emplace_back(static_cast<const VKImageView *>(mCreateInfo.pAttachments[i])->GetHandle());
        if (mCreateInfo.pAttachmentImageInfos) {
            auto &&e = mCreateInfo.pAttachmentImageInfos[i];
            viewFormats[i].resize(e.viewFormatCount);
            for (uint32_t j = 0; j < e.viewFormatCount; ++j) viewFormats[i][j] = Map(e.pViewFormats[j]);

            attachmentImageInfos[i] = {VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO,
                                       0,
                                       Map(e.flags),
                                       Map(e.usage),
                                       e.width,
                                       e.height,
                                       e.layoutCount,
                                       e.viewFormatCount,
                                       viewFormats[i].data()};
        }
    }
    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                                 pNext,
                                 flags,
                                 static_cast<const VKRenderPass *>(mCreateInfo.pRenderPass)->GetHandle(),
                                 static_cast<uint32_t>(attachments.size()),
                                 attachments.data(),
                                 mCreateInfo.extent.width,
                                 mCreateInfo.extent.height,
                                 mCreateInfo.layers};

    if (vkCreateFramebuffer(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create framebuffer");
}
}  // namespace Shit
