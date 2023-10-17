/**
 * @file GLFramebuffer.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLFramebuffer.hpp"

#include "GLImage.hpp"
#include "GLRenderPass.hpp"
#include "GLState.hpp"
namespace Shit {
void GLFramebuffer::FBOState::BindReadBuffer(GLenum mode) {
    if (readBuffer != mode) {
        readBuffer = mode;
        glReadBuffer(mode);
    }
}
void GLFramebuffer::FBOState::BindDrawBuffers(std::span<const GLenum> modes) {
    if (!std::ranges::equal(drawBuffers, modes)) {
        drawBuffers.resize(modes.size());
        std::ranges::copy(modes, drawBuffers.begin());
        glDrawBuffers(static_cast<GLsizei>(modes.size()), modes.data());
    }
}
void GLFramebuffer::FBOState::BindDrawBuffer(GLenum mode) {
    if (drawBuffers.size() != 1 || drawBuffers[0] != mode) {
        drawBuffers.resize(1, mode);
        glDrawBuffer(mode);
    }
}
void GLFramebuffer::FBOState::PushReadBuffer(GLenum mode) {
    readBufferStack.emplace(readBuffer);
    BindReadBuffer(mode);
}
void GLFramebuffer::FBOState::PopReadBuffer() {
    BindReadBuffer(readBufferStack.top());
    readBufferStack.pop();
}
void GLFramebuffer::FBOState::PushDrawBuffers(std::span<const GLenum> modes) {
    drawBuffersStack.emplace(drawBuffers);
    BindDrawBuffers(modes);
}
void GLFramebuffer::FBOState::PopDrawBuffers() {
    BindDrawBuffers(drawBuffersStack.top());
    drawBuffersStack.pop();
}
void GLFramebuffer::FBOState::PushDrawBuffer(GLenum mode) {
    drawBuffersStack.emplace(drawBuffers);
    BindDrawBuffer(mode);
}
void GLFramebuffer::FBOState::PopDrawBuffer() { PopDrawBuffers(); }
void GLFramebuffer::CheckCompleteness() {
    auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) ST_THROW("framebuffe error code:", status);
}
void GLFramebuffer::Resolve(uint32_t fboIndex, Filter filter) const {
    if (mFBOs.at(fboIndex).second) {
        auto renderFBO = &mFBOs.at(fboIndex).first;
        auto resolveFBO = mFBOs.at(fboIndex).second.get();

        // has resolve fbo, need to resolve
        mpStateManager->BindReadFramebuffer(renderFBO->handle);
        mpStateManager->BindDrawFramebuffer(resolveFBO->handle);
        for (size_t i = 0, l = renderFBO->colorAttachments.size(); i < l; ++i) {
            renderFBO->BindReadBuffer(renderFBO->colorAttachments[i].attachPoint);
            resolveFBO->BindDrawBuffer(resolveFBO->colorAttachments[i].attachPoint);

            glBlitFramebuffer(0, 0, mCreateInfo.extent.width, mCreateInfo.extent.height, 0, 0, mCreateInfo.extent.width,
                              mCreateInfo.extent.height, GL_COLOR_BUFFER_BIT, Map(filter));
        }
    }
}
void GLFramebuffer::CreateFBO(uint32_t fboIndex, bool isRenderFBO) {
    auto &&e = mCreateInfo.pRenderPass->GetCreateInfoPtr()->pSubpasses[fboIndex];
    FBOState *fbo;
    const AttachmentReference *attachments;

    if (isRenderFBO) {
        fbo = &mFBOs[fboIndex].first;
        attachments = e.pColorAttachments;
    } else {
        if (!e.pResolveAttachments) return;
        fbo = (mFBOs[fboIndex].second = std::make_unique<FBOState>()).get();
        attachments = e.pResolveAttachments;
    }
    // check default fbo or not
    int testAttachmentIndex = 0;
    if (e.colorAttachmentCount > 0)
        testAttachmentIndex = attachments[0].attachment;
    else if (e.pDepthStencilAttachment)
        testAttachmentIndex = e.pDepthStencilAttachment->attachment;
    else {
        // no attachments
        return;
    }
    auto pImageView = static_cast<const GLImageView *>(mCreateInfo.pAttachments[testAttachmentIndex]);
    auto pImage = static_cast<GLImage const *>(pImageView->GetCreateInfoPtr()->pImage);
    auto imageFlag = pImage->GetImageFlag();

    if (imageFlag == GLImageFlag::TEXTURE || imageFlag == GLImageFlag::RENDERBUFFER) {
        glGenFramebuffers(1, &fbo->handle);
        mpStateManager->BindDrawFramebuffer(fbo->handle);

        // draw color buffers
        fbo->colorAttachments.resize(e.colorAttachmentCount);
        for (uint32_t j = 0; j < e.colorAttachmentCount; ++j) {
            pImageView = static_cast<const GLImageView *>(mCreateInfo.pAttachments[attachments[j].attachment]);
            auto a = GetFormatAttribute(pImageView->GetCreateInfoPtr()->format).formatNumeric;
            auto &fboAttachment = fbo->colorAttachments[j];
            if (a == FormatNumeric::SRGB) {
                fboAttachment.colorEncoding = GL_SRGB;
            }

            pImage = static_cast<GLImage const *>(pImageView->GetCreateInfoPtr()->pImage);
            imageFlag = pImage->GetImageFlag();
            switch (imageFlag) {
                case GLImageFlag::TEXTURE:
                    fboAttachment.objectType = GL_TEXTURE;
                    break;
                case GLImageFlag::RENDERBUFFER:
                    fboAttachment.objectType = GL_RENDERBUFFER;
                    break;
                default:
                    fboAttachment.objectType = GL_FRAMEBUFFER_DEFAULT;
                    break;
            }
            fbo->colorAttachments[j].attachPoint = GL_COLOR_ATTACHMENT0 + j;
            BindFBOAttachment(fbo->colorAttachments[j].attachPoint, pImageView);
        }
        // set drawbuffers and read buffers
        std::vector<GLenum> drawBuffers;
        for (auto aa : fbo->colorAttachments) drawBuffers.emplace_back(aa.attachPoint);
        fbo->BindDrawBuffers(drawBuffers);

        // depthstencil buffer
        if (e.pDepthStencilAttachment && isRenderFBO) {
            auto &fboAttachment = fbo->depthStencilAttachment;
            pImageView =
                static_cast<const GLImageView *>(mCreateInfo.pAttachments[e.pDepthStencilAttachment->attachment]);
            pImage = static_cast<GLImage const *>(pImageView->GetCreateInfoPtr()->pImage);
            imageFlag = pImage->GetImageFlag();
            switch (imageFlag) {
                case GLImageFlag::TEXTURE:
                    fboAttachment.objectType = GL_TEXTURE;
                    break;
                case GLImageFlag::RENDERBUFFER:
                    fboAttachment.objectType = GL_RENDERBUFFER;
                    break;
                default:
                    fboAttachment.objectType = GL_FRAMEBUFFER_DEFAULT;
                    break;
            }

            auto externalFormat = MapBaseFormat(pImageView->GetCreateInfoPtr()->format);
            if (externalFormat == GL_DEPTH_STENCIL)
                fboAttachment.attachPoint = GL_DEPTH_STENCIL_ATTACHMENT;
            else if (externalFormat == GL_DEPTH_COMPONENT)
                fboAttachment.attachPoint = GL_DEPTH_ATTACHMENT;
            else if (externalFormat == GL_STENCIL_INDEX)
                fboAttachment.attachPoint = GL_STENCIL_ATTACHMENT;

            BindFBOAttachment(fbo->depthStencilAttachment.attachPoint, pImageView);
        }

        CheckCompleteness();
    } else {
        // fbo is default fbo, need to set drawbuffers according to imageView
        fbo->colorAttachments.resize(e.colorAttachmentCount);
        for (uint32_t j = 0; j < e.colorAttachmentCount; ++j) {
            pImageView = static_cast<const GLImageView *>(mCreateInfo.pAttachments[attachments[j].attachment]);
            fbo->colorAttachments[j].attachPoint =
                GetFBOBindPoint(static_cast<const GLImage *>(pImageView->GetCreateInfoPtr()->pImage)->GetImageFlag());
        }
        mpStateManager->BindDrawFramebuffer(fbo->handle);
        std::vector<GLenum> drawBuffers;
        for (auto aa : fbo->colorAttachments) drawBuffers.emplace_back(aa.attachPoint);
        fbo->BindDrawBuffers(drawBuffers);
    }
}
void GLFramebuffer::BindFBOAttachment(GLenum bindpoint, const GLImageView *imageView) {
    auto pImage = static_cast<GLImage const *>(imageView->GetCreateInfoPtr()->pImage);
    auto imageFlag = pImage->GetImageFlag();
    if (mMultiview) {
        if (imageFlag == GLImageFlag::TEXTURE) {
            if (pImage->GetCreateInfoPtr()->samples == Shit::SampleCountFlagBits::BIT_1) {
                glFramebufferTextureMultiviewOVR(GL_DRAW_FRAMEBUFFER, bindpoint, imageView->GetHandle(), 0, 0,
                                                 imageView->GetCreateInfoPtr()->subresourceRange.layerCount);
            } else {
                glFramebufferTextureMultisampleMultiviewOVR(GL_DRAW_FRAMEBUFFER, bindpoint, imageView->GetHandle(), 0,
                                                            (GLsizei)(pImage->GetCreateInfoPtr()->samples), 0,
                                                            imageView->GetCreateInfoPtr()->subresourceRange.layerCount);
            }
        } else {
            ST_THROW("must use a texture to use multiview")
        }
        return;
    }

    switch (imageFlag) {
        case GLImageFlag::TEXTURE:
            glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindpoint, imageView->GetHandle(),
                                 imageView->GetCreateInfoPtr()->subresourceRange.baseMipLevel);
            break;
        case GLImageFlag::RENDERBUFFER:
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindpoint, GL_RENDERBUFFER, imageView->GetHandle());
            break;
        default:
            break;
    }
}
GLFramebuffer::GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo)
    : Framebuffer(createInfo), mpStateManager(pStateManager) {
    if (mCreateInfo.pRenderPass->GetCreateInfoPtr()->pMultiviewCreateInfo) {
        static bool a = glIsExtensionSupported("GL_OVR_multiview");
        if (a) {
            mMultiview = true;
        } else {
            ST_LOG("GL_OVR_multiview is not supported, cannot use multiview ")
        }
        if (mCreateInfo.pRenderPass->GetCreateInfoPtr()->pAttachments[0].samples != Shit::SampleCountFlagBits::BIT_1) {
            static bool b = glIsExtensionSupported("GL_OVR_multiview_multisampled_render_to_texture");
            if (b) {
                mMultiview = true;
            } else {
                ST_LOG(
                    "GL_OVR_multiview_multisampled_render_to_texture is not supported, "
                    "cannot use multiview ")
            }
        }
    }

    // subpass count
    auto count = mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpassCount;
    mFBOs.resize(count);

    for (uint32_t i = 0; i < count; ++i) {
        CreateFBO(i, true);   // renderfbos
        CreateFBO(i, false);  // create resolve fbos
    }
}
GLFramebuffer::~GLFramebuffer() {
    for (auto &&e : mFBOs) {
        if (e.first.handle) {
            mpStateManager->NotifyReleasedFramebuffer(e.first.handle);
            glDeleteFramebuffers(1, &e.first.handle);
        }
        if (e.second && e.second->handle) {
            mpStateManager->NotifyReleasedFramebuffer(e.second->handle);
            glDeleteFramebuffers(1, &e.second->handle);
        }
    }
}
void GLFramebuffer::BindFBO(uint32_t fboIndex, bool isRenderFBO, bool isDraw) const {
    const FBOState *fbo = isRenderFBO ? &mFBOs.at(fboIndex).first : mFBOs.at(fboIndex).second.get();
    if (isDraw)
        mpStateManager->BindDrawFramebuffer(fbo->handle);
    else
        mpStateManager->BindReadFramebuffer(fbo->handle);

    if (!fbo->colorAttachments.empty()) {
        if (fbo->colorAttachments[0].colorEncoding == GL_SRGB)
            mpStateManager->EnableCapability(GL_FRAMEBUFFER_SRGB);
        else
            mpStateManager->DisableCapability(GL_FRAMEBUFFER_SRGB);
    }
    ClearBuffer(fboIndex);
}
void GLFramebuffer::ClearBuffer(uint32_t fboIndex) const {
    // mpStateManager->PushDrawFramebuffer(fbo->handle);
    // clear color attachment
    auto &&attachments = mCreateInfo.pRenderPass->GetCreateInfoPtr()->pAttachments;
    auto &&subpassDesc = mCreateInfo.pRenderPass->GetCreateInfoPtr()->pSubpasses[fboIndex];
    for (uint32_t i = 0; i < subpassDesc.colorAttachmentCount; ++i) {
        auto &&e = subpassDesc.pColorAttachments[i];
        if (attachments[e.attachment].loadOp == AttachmentLoadOp::CLEAR) {
            auto formatNumeric = GetFormatAttribute(attachments[e.attachment].format).formatNumeric;
            auto v =
                static_cast<GLRenderPass const *>(mCreateInfo.pRenderPass)->GetAttachmentClearValuePtr(e.attachment);

            if (formatNumeric == FormatNumeric::SINT)
                glClearBufferiv(GL_COLOR, i, reinterpret_cast<const int32_t *>(std::get_if<ClearColorValueInt32>(v)));
            else if (formatNumeric == FormatNumeric::UINT)
                glClearBufferuiv(GL_COLOR, i,
                                 reinterpret_cast<const uint32_t *>(std::get_if<ClearColorValueUint32>(v)));
            else
                glClearBufferfv(GL_COLOR, i, reinterpret_cast<const float *>(std::get_if<ClearColorValueFloat>(v)));
        }
    }
    // clear depth stencil attachment
    if (subpassDesc.pDepthStencilAttachment) {
        auto attachmentIndex = subpassDesc.pDepthStencilAttachment->attachment;
        auto &&formatAttrib = GetFormatAttribute(attachments[attachmentIndex].format);

        int target = 0;
        if (attachments[attachmentIndex].loadOp == AttachmentLoadOp::CLEAR) target |= 1;
        if (attachments[attachmentIndex].stencilLoadOp == AttachmentLoadOp::CLEAR) target |= 2;
        if (target > 0) {
            auto clearValue =
                static_cast<GLRenderPass const *>(mCreateInfo.pRenderPass)->GetAttachmentClearValuePtr(attachmentIndex);
            int flag = 0;
            flag |= (formatAttrib.componentSizeDepth > 0);
            flag |= (formatAttrib.componentSizeStencil > 0) << 1;

            if (flag == 1)
                glClearBufferfv(GL_DEPTH, 0, &std::get_if<ClearDepthStencilValue>(clearValue)->depth);
            else if (flag == 2)
                glClearBufferuiv(GL_STENCIL, 0, &std::get_if<ClearDepthStencilValue>(clearValue)->stencil);
            else if (flag == 3)
                glClearBufferfi(GL_DEPTH_STENCIL, 0, std::get_if<ClearDepthStencilValue>(clearValue)->depth,
                                std::get_if<ClearDepthStencilValue>(clearValue)->stencil);
            else
                ST_THROW("failed to find clear value of attachment");
        }
    }
    // mpStateManager->PopDrawFramebuffer();
}
// TODO: has problem when not set all masks
void GLFramebuffer::SetColorMask(uint32_t fboIndex, uint32_t attachmentIndex, GLuint writeMask) {
    if (mFBOs[fboIndex].first.colorAttachments[attachmentIndex].writeMask != writeMask) {
        glColorMaski(attachmentIndex, writeMask & 0x1, writeMask & 0x2, writeMask & 0x4, writeMask & 0x8);
        mFBOs[fboIndex].first.colorAttachments[attachmentIndex].writeMask = writeMask;
    }
}
void GLFramebuffer::SetDepthMask(uint32_t fboIndex, bool writeMask) {
    if (mFBOs[fboIndex].first.depthStencilAttachment.depthWriteMask != writeMask) {
        glDepthMask(writeMask);
        mFBOs[fboIndex].first.depthStencilAttachment.depthWriteMask = writeMask;
    }
}
void GLFramebuffer::SetStencilMask(uint32_t fboIndex, GLuint writeMask) {
    if (mFBOs[fboIndex].first.depthStencilAttachment.stencilWriteMask != writeMask) {
        glStencilMaskSeparate(GL_FRONT, writeMask);
        mFBOs[fboIndex].first.depthStencilAttachment.stencilWriteMask = writeMask;
    }
}
void GLFramebuffer::SetStencilBackMask(uint32_t fboIndex, GLuint writeMask) {
    if (mFBOs[fboIndex].first.depthStencilAttachment.stencilBackWriteMask != writeMask) {
        glStencilMaskSeparate(GL_BACK, writeMask);
        mFBOs[fboIndex].first.depthStencilAttachment.stencilBackWriteMask = writeMask;
    }
}
}  // namespace Shit
