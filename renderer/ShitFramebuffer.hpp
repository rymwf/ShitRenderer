/**
 * @file ShitFramebuffer.hpp
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
/**
 * @brief framebuffer contains a collection of attachments,
 * but which attachment be used is defined by renderpass
 *
 */
class Framebuffer {
protected:
    FramebufferCreateInfo mCreateInfo;
    Framebuffer(const FramebufferCreateInfo &createInfo) : mCreateInfo(createInfo) {
        if (createInfo.attachmentCount) {
            if (mCreateInfo.pAttachments) {
                mCreateInfo.pAttachments = new const ImageView *[createInfo.attachmentCount];
                memcpy((void *)mCreateInfo.pAttachments, createInfo.pAttachments,
                       sizeof(std::ptrdiff_t) * mCreateInfo.attachmentCount);
            } else {
                mCreateInfo.pAttachmentImageInfos = new FramebufferAttachmentImageInfo[createInfo.attachmentCount];
                memcpy((void *)mCreateInfo.pAttachmentImageInfos, createInfo.pAttachmentImageInfos,
                       sizeof(FramebufferAttachmentImageInfo) * mCreateInfo.attachmentCount);
                for (uint32_t i = 0; i < createInfo.attachmentCount; ++i) {
                    auto p = const_cast<FramebufferAttachmentImageInfo *>(&mCreateInfo.pAttachmentImageInfos[i]);
                    if (p->viewFormatCount) {
                        p->pViewFormats = new Format[p->viewFormatCount];
                        memcpy((void *)p->pViewFormats, createInfo.pAttachmentImageInfos[i].pViewFormats,
                               sizeof(Format) * p->viewFormatCount);
                    }
                }
            }
        }
    }

public:
    virtual ~Framebuffer() {
        delete[] mCreateInfo.pAttachments;
        if (mCreateInfo.pAttachmentImageInfos) {
            for (uint32_t i = 0; i < mCreateInfo.attachmentCount; ++i) {
                delete[] mCreateInfo.pAttachmentImageInfos[i].pViewFormats;
            }
        }
    }
    constexpr const FramebufferCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
};
}  // namespace Shit
