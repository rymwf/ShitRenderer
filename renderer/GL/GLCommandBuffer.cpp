/**
 * @file GLCommandBuffer.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLCommandBuffer.hpp"

#include "GLBuffer.hpp"
#include "GLBufferView.hpp"
#include "GLCommandPool.hpp"
#include "GLDescriptor.hpp"
#include "GLFramebuffer.hpp"
#include "GLImage.hpp"
#include "GLPipeline.hpp"
#include "GLRenderPass.hpp"
#include "GLState.hpp"

namespace Shit {
GLCommandBuffer::DummyFramebuffer::DummyFramebuffer() {
    glGenFramebuffers(1, &_handle);
    _colorAttachmentLevels.fill(~0u);
}
GLCommandBuffer::DummyFramebuffer::~DummyFramebuffer() { glDeleteFramebuffers(1, &_handle); }
void GLCommandBuffer::DummyFramebuffer::ImageDestroyListener(GLImage const *pImage) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _handle);
    if (_depthStencilAttachment == pImage) {
        glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
        _depthStencilAttachment = 0;
    } else if (_colorAttachment == pImage) {
        for (uint32_t i = 0; i < _colorAttachmentLevels.size(); ++i) {
            if (_colorAttachmentLevels[i] != ~0u) {
                glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, 0);
                _colorAttachmentLevels[i] = ~0u;
            }
        }
        _colorAttachment = 0;
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}
GLuint GLCommandBuffer::DummyFramebuffer::BindImage(GLStateManager *pManager, GLImage const *pImage,
                                                    ImageAspectFlagBits imageAspect, uint32_t levelCount,
                                                    uint32_t const *levels) {
    bool bound = false;
    auto pGLImage = static_cast<const GLImage *>(pImage);
    auto imageFlag = pGLImage->GetImageFlag();
    // check if already bound
    if (_depthStencilAttachment == pImage && _depthStencilLevel == levels[0]) {
        bound = true;
    }
    if (!bound && _colorAttachment == pImage) {
        for (uint32_t i = 0; i < levelCount; ++i) {
            if (levels[i] == _colorAttachmentLevels[i]) bound = true;
        }
    }
    if (bound) {
        if (imageFlag == GLImageFlag::TEXTURE || imageFlag == GLImageFlag::RENDERBUFFER) return _handle;
        return 0;
    }
    if (imageFlag != GLImageFlag::TEXTURE && imageFlag != GLImageFlag::RENDERBUFFER) return 0;

    // bind
    GLenum bindPoint = GL_DEPTH_STENCIL_ATTACHMENT;
    bool isDepth = true;
    switch (imageAspect) {
        case ImageAspectFlagBits::DEPTH_BIT:
            bindPoint = GL_DEPTH_ATTACHMENT;
            break;
        case ImageAspectFlagBits::STENCIL_BIT:
            bindPoint = GL_STENCIL_ATTACHMENT;
            break;
        case ImageAspectFlagBits::COLOR_BIT:
            bindPoint = GL_COLOR_ATTACHMENT0;
            isDepth = false;
            break;
    }
    pImage->AddDestroySignalListener(
        std::bind(&GLCommandBuffer::DummyFramebuffer::ImageDestroyListener, this, std::placeholders::_1));
    pManager->PushDrawFramebuffer(_handle);
    if (isDepth) {
        _depthStencilAttachment = pImage;
        if (_colorAttachment) {
            for (uint32_t i = 0; i < 16; ++i) {
                if (_colorAttachmentLevels[i] != ~0u) {
                    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_RENDERBUFFER, 0);
                    _colorAttachmentLevels[i] = ~0u;
                }
            }
            _colorAttachment->RemoveDestroySignalListener(
                std::bind(&GLCommandBuffer::DummyFramebuffer::ImageDestroyListener, this, std::placeholders::_1));
        }
    } else {
        _colorAttachment = pImage;
        if (_depthStencilAttachment) {
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
            _depthStencilAttachment->RemoveDestroySignalListener(
                std::bind(&GLCommandBuffer::DummyFramebuffer::ImageDestroyListener, this, std::placeholders::_1));
            _depthStencilLevel = ~0u;
            _depthStencilAttachment = 0;
        }
    }

    std::vector<GLenum> attachpoints(levelCount);
    switch (imageFlag) {
        case GLImageFlag::TEXTURE: {
            for (uint32_t i = 0; i < levelCount; ++i) {
                attachpoints[i] = bindPoint + i;
                glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindPoint + i, pGLImage->GetHandle(), levels[i]);
                if (isDepth)
                    _depthStencilLevel = levels[i];
                else
                    _colorAttachmentLevels[i] = levels[i];
            }
            break;
        }
        case GLImageFlag::RENDERBUFFER: {
            glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindPoint, GL_RENDERBUFFER, pGLImage->GetHandle());
            if (isDepth)
                _depthStencilLevel = levels[0];
            else {
                attachpoints[0] = bindPoint;
                _colorAttachmentLevels[0] = levels[0];
            }
            break;
        }
    }
    if (bindPoint == GL_COLOR_ATTACHMENT0) glDrawBuffers(levelCount, attachpoints.data());
    {
        auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) ST_THROW("framebuffe error code:", status);
    }
    pManager->PopDrawFramebuffer();
    return _handle;
}

GLCommandBuffer::CommandFunctionPtr GLCommandBuffer::mCommandFunctions[]{
    nullptr,                                          // BEGIN_QUERY
    &GLCommandBuffer::_BeginRenderPass,               // BEGIN_RENDERPASS
    &GLCommandBuffer::_BeginTransformFeedback,        // BEGIN_TRANSFORMFEEDBACK
    &GLCommandBuffer::_BindDescriptorSets,            // BIND_DESCRIPTORSETS
    &GLCommandBuffer::_BindIndexBuffer,               // BIND_INDEXBUFFER
    &GLCommandBuffer::_BindPipeline,                  // BIND_PIPELINE
    &GLCommandBuffer::_BindTransformFeedBackBuffers,  // BIND_TRANSFORMFEEDBACK_BUFFERS
    &GLCommandBuffer::_BindVertexBuffers,             // BIND_VERTEX_BUFFER
    &GLCommandBuffer::_BlitImage,                     // BLIT_IMAGE
    &GLCommandBuffer::_ClearAttachments,              // CLEAR_ATTACHMENTS
    &GLCommandBuffer::_ClearColorImage,               // CLEAR_COLOR_IMAGE
    &GLCommandBuffer::_ClearDepthStencilImage,        // CLEAR_DEPTH_STENCIL_IMAGE
    &GLCommandBuffer::_CopyBuffer,                    // COPY_BUFFER
    &GLCommandBuffer::_CopyBufferToImage,             // COPY_BUFFER_TO_IMAGE
    &GLCommandBuffer::_CopyImage,                     // COPY_IMAGE
    &GLCommandBuffer::_CopyImageToBuffer,             // COPY_IMAGE_TO_BUFFER
    nullptr,                                          // COPY_QUERYPOOL_RESULTS
    &GLCommandBuffer::_Dispatch,                      // DISPATCH
    &GLCommandBuffer::_DispatchIndirect,              // DISPATCH_INDIRECT
    &GLCommandBuffer::_Draw,                          // DRAW
    &GLCommandBuffer::_DrawIndirect,                  // DRAW_INDIRECT
    &GLCommandBuffer::_DrawIndirectCount,             // DRAW_INDIRECT_COUNT
    &GLCommandBuffer::_DrawIndexed,                   // DRAW_INDEXED
    &GLCommandBuffer::_DrawIndexedIndirect,           // DRAW_INDEXED_INDIRECT
    &GLCommandBuffer::_DrawIndexedIndirectCount,      // DRAW_INDEXED_INDIRECT_COUNT
    nullptr,                                          // END_QUERY
    &GLCommandBuffer::_EndRenderPass,                 // END_RENDERPASS
    &GLCommandBuffer::_EndTransformFeedback,          // END_TRANSFORMFEEDBACK
    &GLCommandBuffer::_ExecuteCommands,               // EXECUTE_COMMANDS
    &GLCommandBuffer::_FillBuffer,                    // FILL_BUFFER
    &GLCommandBuffer::_GenerateMipmap,                // GENERATE_MIPMAP
    &GLCommandBuffer::_NextSubpass,                   // NEXT_SUBPASS
    &GLCommandBuffer::_PipelineBarrier,               // PIPELINE_BARRIER
    &GLCommandBuffer::_PushConstants,                 // PUSH_CONSTANTS
    nullptr,                                          // RESET_EVENT
    nullptr,                                          // RESET_QUERYPOOL
    &GLCommandBuffer::_ResolveImage,                  // RESOLVE_IMAGE
    &GLCommandBuffer::_SetBlendConstants,             // SET_BLEND_CONSTANTS
    &GLCommandBuffer::_SetDepthBias,                  // SET_DEPTH_BIAS
    &GLCommandBuffer::_SetDepthBounds,                // SET_DEPTH_BOUNDS
    nullptr,                                          // SET_DEVICE_MASK
    nullptr,                                          // SET_EVENT
    &GLCommandBuffer::_SetLineWidth,                  // SET_LINE_WIDTH
    &GLCommandBuffer::_SetScissor,                    // SET_SCISSOR
    &GLCommandBuffer::_SetStencilCompareMask,         // SET_STENCIL_COMPARE_MASK
    &GLCommandBuffer::_SetStencilReference,           // SET_STENCIL_REFERENCE
    &GLCommandBuffer::_SetStencilWriteMask,           // SET_STENCIL_WRITE_MASK
    &GLCommandBuffer::_SetViewport,                   // SET_VIEWPORT
    &GLCommandBuffer::_UpdateBuffer,                  // UPDATE_BUFFER
    nullptr,                                          // WAIT_EVENT
    nullptr,                                          // WRITE_TIMESTAMP
};

GLCommandBuffer::GLCommandBuffer(GLStateManager *pStateManager, GLCommandPool *pCommandPool, CommandBufferLevel level)
    : CommandBuffer(pCommandPool, level), mpStateManager(pStateManager) {}
GLCommandBuffer::~GLCommandBuffer() {}
// size_t GLCommandBuffer::_Begin(const void *params)
//{
//	auto cmd = reinterpret_cast<const CommandBufferBeginInfo *>(params);
//	if (cmd->inheritanceInfo.pRenderPass)
//	{
//		mCurRenderPass = cmd->inheritanceInfo.pRenderPass;
//		mCurSubpass = cmd->inheritanceInfo.subpass;
//		mpCurFramebuffer = static_cast<GLFramebuffer
//*>(cmd->inheritanceInfo.pFramebuffer);
//	}
//	return sizeof(*cmd);
// }
size_t GLCommandBuffer::_BeginRenderPass(const void *params) {
    auto cmd = reinterpret_cast<const BeginRenderPassInfo *>(params);
    mpCurFramebuffer = const_cast<GLFramebuffer *>(static_cast<GLFramebuffer const *>(cmd->pFramebuffer));
    mCurRenderPass = cmd->pRenderPass;
    mCurSubpass = 0;

    mpCurFramebuffer->BindFBO(mCurSubpass, true, true);
    // TODO: what is renderArea, handle subpass dependency
    // clear value
    return offsetof(BeginRenderPassInfo, clearValueCount);
}
size_t GLCommandBuffer::_BeginTransformFeedback(const void *params) {
    // TODO: _BeginTransformFeedback
    auto cmd = reinterpret_cast<const BeginTransformFeedbackInfo *>(params);
    ST_LOG("BeginTransformFeedback not implemented yet");
    return offsetof(BeginTransformFeedbackInfo, ppCounterBuffers) +
           (sizeof(ptrdiff_t) + sizeof(uint64_t)) * cmd->counterBufferCount;
}
size_t GLCommandBuffer::_BindIndexBuffer(const void *params) {
    auto cmd = reinterpret_cast<const BindIndexBufferInfo *>(params);
    mpStateManager->BindIndexBuffer(BindIndexBufferInfo{cmd->pBuffer, cmd->offset, cmd->indexType});
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_BindPipeline(const void *params) {
    auto cmd = reinterpret_cast<const BindPipelineInfo *>(params);
    auto pPipeline = dynamic_cast<GLPipeline const *>(cmd->pPipeline);
    if (pPipeline) {
        mpStateManager->BindPipeline(pPipeline);

        // mpStateManager->BindPipeline(pPipeline->GetHandle());
        if (cmd->bindPoint == PipelineBindPoint::GRAPHICS) {
            auto pGraphicsPipeline = static_cast<GLGraphicsPipeline const *>(pPipeline);
            auto &&pGraphicsPipelineCreateInfo = pGraphicsPipeline->GetCreateInfoPtr();

            // dyanmic states
            std::vector<bool> dynamicFlags(static_cast<size_t>(DynamicState::Num));
            for (uint32_t i = 0; i < pGraphicsPipelineCreateInfo->dynamicState.dynamicStateCount; ++i) {
                dynamicFlags[static_cast<size_t>(pGraphicsPipelineCreateInfo->dynamicState.dynamicStates[i])] = true;
            }
            // primitive
            if (!dynamicFlags[static_cast<size_t>(DynamicState::PRIMITIVE_TOPOSTLOGY)])
                mpStateManager->PrimitiveTopology(Map(pGraphicsPipelineCreateInfo->inputAssemblyState.topology));
            if (pGraphicsPipelineCreateInfo->inputAssemblyState.primitiveRestartEnable)
                mpStateManager->EnablePrimitiveRestart();
            else
                mpStateManager->DisablePrimitiveRestart();

            // view port
            if (!dynamicFlags[static_cast<size_t>(DynamicState::VIEWPORT)]) {
                mpStateManager->SetViewports(0, pGraphicsPipelineCreateInfo->viewportState.viewportCount,
                                             (float *)pGraphicsPipelineCreateInfo->viewportState.viewports);
            }

            // tessllation
            mpStateManager->PatchInputVertexNum(
                static_cast<GLint>(pGraphicsPipelineCreateInfo->tessellationState.patchControlPoints));

            // scissor
            if (!dynamicFlags[static_cast<size_t>(DynamicState::SCISSOR)]) {
                mpStateManager->EnableCapability(GL_SCISSOR_TEST);
                mpStateManager->SetScissors(0, pGraphicsPipelineCreateInfo->viewportState.scissorCount,
                                            (int32_t *)pGraphicsPipelineCreateInfo->viewportState.scissors);
            } else
                mpStateManager->DisableCapability(GL_SCISSOR_TEST);

            // rasterization
            if (pGraphicsPipelineCreateInfo->rasterizationState.depthClampEnable)
                mpStateManager->EnableCapability(GL_DEPTH_CLAMP);  // no z clip
            else
                mpStateManager->DisableCapability(GL_DEPTH_CLAMP);

            if (pGraphicsPipelineCreateInfo->rasterizationState.rasterizerDiscardEnbale)
                mpStateManager->EnableCapability(GL_RASTERIZER_DISCARD);
            else
                mpStateManager->DisableCapability(GL_RASTERIZER_DISCARD);

            mpStateManager->PolygonMode(Map(pGraphicsPipelineCreateInfo->rasterizationState.polygonMode));

            if (!dynamicFlags[static_cast<size_t>(DynamicState::CULL_MODE)] &&
                pGraphicsPipelineCreateInfo->rasterizationState.cullMode != CullMode::NONE) {
                mpStateManager->EnableCapability(GL_CULL_FACE);
                mpStateManager->CullFace(Map(pGraphicsPipelineCreateInfo->rasterizationState.cullMode));
            } else
                mpStateManager->DisableCapability(GL_CULL_FACE);

            if (!dynamicFlags[static_cast<size_t>(DynamicState::FRONT_FACE)])
                mpStateManager->FrontFace(Map(pGraphicsPipelineCreateInfo->rasterizationState.frontFace));

            // depth biase
            if (pGraphicsPipelineCreateInfo->rasterizationState.depthBiasEnable) {
                if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::FILL)
                    mpStateManager->EnableCapability(GL_POLYGON_OFFSET_FILL);
                else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::LINE)
                    mpStateManager->EnableCapability(GL_POLYGON_OFFSET_LINE);
                else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::POINT)
                    mpStateManager->EnableCapability(GL_POLYGON_OFFSET_POINT);

                mpStateManager->PolygonOffSetClamp(
                    pGraphicsPipelineCreateInfo->rasterizationState.depthBiasContantFactor,
                    pGraphicsPipelineCreateInfo->rasterizationState.depthBiasSlopeFactor,
                    pGraphicsPipelineCreateInfo->rasterizationState.depthBiasClamp);
            } else {
                if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::FILL)
                    mpStateManager->DisableCapability(GL_POLYGON_OFFSET_FILL);
                else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::LINE)
                    mpStateManager->DisableCapability(GL_POLYGON_OFFSET_LINE);
                else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::POINT)
                    mpStateManager->DisableCapability(GL_POLYGON_OFFSET_POINT);
                mpStateManager->PolygonOffSetClamp(0.f, 0.f, 0.f);
            }
            // line width
            if (!dynamicFlags[static_cast<size_t>(DynamicState::LINE_WIDTH)])
                mpStateManager->LineWidth(pGraphicsPipelineCreateInfo->rasterizationState.lineWidth);
            // multisample
            if (pGraphicsPipelineCreateInfo->multisampleState.sampleShadingEnable) {
                mpStateManager->EnableCapability(GL_SAMPLE_SHADING);
                mpStateManager->MinSampleShading(pGraphicsPipelineCreateInfo->multisampleState.minSampleShading);
                // TODO: sample mask

                if (pGraphicsPipelineCreateInfo->multisampleState.alphaToCoverageEnable)
                    mpStateManager->EnableCapability(GL_SAMPLE_ALPHA_TO_COVERAGE);
                else
                    mpStateManager->DisableCapability(GL_SAMPLE_ALPHA_TO_COVERAGE);

                if (pGraphicsPipelineCreateInfo->multisampleState.alphaToOneEnable)
                    mpStateManager->EnableCapability(GL_SAMPLE_ALPHA_TO_ONE);
                else
                    mpStateManager->DisableCapability(GL_SAMPLE_ALPHA_TO_ONE);
            } else {
                mpStateManager->DisableCapability(GL_SAMPLE_SHADING);
            }

            // depth test
            if (pGraphicsPipelineCreateInfo->depthStencilState.depthTestEnable) {
                mpStateManager->EnableCapability(GL_DEPTH_TEST);
                mpStateManager->DepthFunc(Map(pGraphicsPipelineCreateInfo->depthStencilState.depthCompareOp));
                // mpCurFramebuffer->SetDepthMask(mCurSubpass,
                // pGraphicsPipelineCreateInfo->depthStencilState.depthWriteEnable);
            } else {
                mpStateManager->DisableCapability(GL_DEPTH_TEST);
                // mpCurFramebuffer->SetDepthMask(mCurSubpass, false);
            }
            // stencil test
            if (pGraphicsPipelineCreateInfo->depthStencilState.stencilTestEnable) {
                mpStateManager->EnableCapability(GL_STENCIL_TEST);
                // stencil op
                mpStateManager->StencilOpState(GL_FRONT,
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.front.compareOp),
                                               pGraphicsPipelineCreateInfo->depthStencilState.front.reference,
                                               pGraphicsPipelineCreateInfo->depthStencilState.front.compareMask,
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.front.failOp),
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.front.depthFailOp),
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.front.passOp));
                mpStateManager->StencilOpState(GL_BACK,
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.back.compareOp),
                                               pGraphicsPipelineCreateInfo->depthStencilState.back.reference,
                                               pGraphicsPipelineCreateInfo->depthStencilState.back.compareMask,
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.back.failOp),
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.back.depthFailOp),
                                               Map(pGraphicsPipelineCreateInfo->depthStencilState.back.passOp));
                // mpCurFramebuffer->SetStencilMask(mCurSubpass,
                // pGraphicsPipelineCreateInfo->depthStencilState.front.writeMask);
                // mpCurFramebuffer->SetStencilBackMask(mCurSubpass,
                // pGraphicsPipelineCreateInfo->depthStencilState.back.writeMask);
            } else {
                mpStateManager->DisableCapability(GL_STENCIL_TEST);
                // mpCurFramebuffer->SetStencilMask(mCurSubpass, false);
                // mpCurFramebuffer->SetStencilBackMask(mCurSubpass, false);
            }

            // blend
            for (uint32_t i = 0; i < pGraphicsPipelineCreateInfo->colorBlendState.attachmentCount; ++i) {
                auto &&e = pGraphicsPipelineCreateInfo->colorBlendState.attachments[i];
                if (e.blendEnable) {
                    mpStateManager->EnableBlend(i);
                    mpStateManager->BlendEquation(i, Map(e.colorBlendOp), Map(e.alphaBlendOp));
                    mpStateManager->BlendFunc(i, Map(e.srcColorBlendFactor), Map(e.dstColorBlendFactor),
                                              Map(e.srcAlphaBlendFactor), Map(e.dstAlphaBlendFactor));
                } else {
                    mpStateManager->DisableBlend(i);
                }
                // mpCurFramebuffer->SetColorMask(mCurSubpass, i,
                // (GLuint)e.colorWriteMask);
            }
            if (pGraphicsPipelineCreateInfo->colorBlendState.logicOpEnable) {
                mpStateManager->EnableCapability(GL_COLOR_LOGIC_OP);
                mpStateManager->LogicOp(Map(pGraphicsPipelineCreateInfo->colorBlendState.logicOp));
            } else
                mpStateManager->DisableCapability(GL_COLOR_LOGIC_OP);
            mpStateManager->BlendColor(pGraphicsPipelineCreateInfo->colorBlendState.blendConstants);
        }
    }
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_BindVertexBuffers(const void *params) {
    auto cmd = reinterpret_cast<const BindVertexBuffersInfo *>(params);
    auto ppBuffers = reinterpret_cast<const Buffer *const *>(&cmd->ppBuffers);
    auto pOffsets = reinterpret_cast<const uint64_t *>(&cmd->ppBuffers + cmd->bindingCount);
    mpStateManager->BindVertexBuffers(BindVertexBuffersInfo{cmd->firstBinding, cmd->bindingCount, ppBuffers, pOffsets});
    return offsetof(BindVertexBuffersInfo, ppBuffers) + (sizeof(uint64_t) + sizeof(ptrdiff_t)) * cmd->bindingCount;
}
size_t GLCommandBuffer::_BindDescriptorSets(const void *params) {
    auto cmd = reinterpret_cast<const BindDescriptorSetsInfo *>(params);
    auto pDescriptorSets = reinterpret_cast<const GLDescriptorSet *const *>(&cmd->ppDescriptorSets);
    auto dynamicOffsetCount = *reinterpret_cast<const uint32_t *>(&pDescriptorSets[cmd->descriptorSetCount]);
    auto pDynamicOffsets = reinterpret_cast<const uint32_t *>(&dynamicOffsetCount + 1);
    uint32_t dynamicOffsetIndex = 0;
    for (uint32_t i = 0; i < cmd->descriptorSetCount; ++i) {
        const_cast<GLDescriptorSet *>(pDescriptorSets[i])->Bind(pDynamicOffsets, dynamicOffsetIndex);
    }
    return offsetof(BindDescriptorSetsInfo, ppDescriptorSets) + sizeof(ptrdiff_t) * cmd->descriptorSetCount +
           sizeof(uint32_t) * (size_t)(cmd->dynamicOffsetCount + 1);
}
size_t GLCommandBuffer::_BindTransformFeedBackBuffers(const void *params) {
    auto cmd = reinterpret_cast<const BindTransformFeedbackBuffersInfo *>(params);
    ST_LOG("BindTransformFeedBackBuffers not implemented yet");
    // auto ppBuffers = reinterpret_cast<Buffer *const *>(&cmd->ppBuffers);
    // auto pOffsets = reinterpret_cast<const uint64_t *>(&cmd->ppBuffers +
    // cmd->bindingCount); auto pSizes = reinterpret_cast<const uint64_t
    // *>(&cmd->ppBuffers + cmd->bindingCount) + cmd->bindingCount;

    return offsetof(BindTransformFeedbackBuffersInfo, ppBuffers) +
           (sizeof(ptrdiff_t) + sizeof(uint64_t) * 2) * cmd->bindingCount;
}
size_t GLCommandBuffer::_BlitImage(const void *params) {
    auto cmd = reinterpret_cast<const BlitImageInfo *>(params);
    auto blitRegions = reinterpret_cast<const ImageBlit *>(&cmd->pRegions);
    std::vector<uint32_t> srcLevels(cmd->regionCount);
    std::vector<uint32_t> dstLevels(cmd->regionCount);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        srcLevels[i] = blitRegions[i].srcSubresource.mipLevel;
        dstLevels[i] = blitRegions[i].dstSubresource.mipLevel;
    }
    auto srcFBO = mDummyFramebuffer.BindImage(mpStateManager, static_cast<GLImage const *>(cmd->pSrcImage),
                                              ImageAspectFlagBits::COLOR_BIT, cmd->regionCount, srcLevels.data());
    auto dstFBO = mDummyFramebuffer2.BindImage(mpStateManager, static_cast<GLImage const *>(cmd->pDstImage),
                                               ImageAspectFlagBits::COLOR_BIT, cmd->regionCount, dstLevels.data());
    mpStateManager->PushReadFramebuffer(srcFBO);
    mpStateManager->PushDrawFramebuffer(dstFBO);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        auto &&e = cmd->pRegions[i];
        GLenum mask = 0;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::COLOR_BIT)) mask |= GL_COLOR_BUFFER_BIT;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::DEPTH_BIT)) mask |= GL_DEPTH_BUFFER_BIT;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::STENCIL_BIT)) mask |= GL_STENCIL_BUFFER_BIT;
        glBlitFramebuffer(e.srcOffsets[0].x, e.srcOffsets[0].y, e.srcOffsets[1].x, e.srcOffsets[1].y, e.dstOffsets[0].x,
                          e.dstOffsets[0].y, e.dstOffsets[1].x, e.dstOffsets[1].y, mask, GL_LINEAR);
    }
    mpStateManager->PopReadFramebuffer();
    mpStateManager->PopDrawFramebuffer();
    return offsetof(BlitImageInfo, pRegions) + sizeof(ImageBlit) * cmd->regionCount;
}
size_t GLCommandBuffer::_ClearColorImage(const void *params) {
    auto cmd = reinterpret_cast<const ClearColorImageInfo *>(params);
    auto pImage = static_cast<const GLImage *>(cmd->image);
    auto pRegions = reinterpret_cast<const ImageSubresourceRange *>(&cmd->pRanges);

    std::vector<uint32_t> levels;
    for (uint32_t i = 0; i < cmd->rangeCount; ++i) {
        auto &&e = pRegions[i];
        for (uint32_t j = 0; j < e.levelCount; ++j) {
            levels.emplace_back(pRegions[i].baseMipLevel + j);
        }
    }
    auto fbo = mDummyFramebuffer.BindImage(mpStateManager, pImage, ImageAspectFlagBits::COLOR_BIT,
                                           (uint32_t)levels.size(), levels.data());
    mpStateManager->PushDrawFramebuffer(fbo);
    if (auto p1 = std::get_if<ClearColorValueFloat>(&cmd->clearColor)) {
        glClearBufferfv(GL_COLOR, 0, p1->data());
    } else if (auto p2 = std::get_if<ClearColorValueInt32>(&cmd->clearColor)) {
        glClearBufferiv(GL_COLOR, 0, p2->data());
    } else if (auto p3 = std::get_if<ClearColorValueUint32>(&cmd->clearColor)) {
        glClearBufferuiv(GL_COLOR, 0, p3->data());
    }
    mpStateManager->PopDrawFramebuffer();
    return offsetof(ClearColorImageInfo, pRanges) + sizeof(ImageSubresourceRange) * cmd->rangeCount;
}
size_t GLCommandBuffer::_ClearDepthStencilImage(const void *params) {
    auto cmd = reinterpret_cast<const ClearDepthStencilImageInfo *>(params);
    auto pImage = static_cast<const GLImage *>(cmd->image);
    auto pRegions = reinterpret_cast<const ImageSubresourceRange *>(&cmd->pRanges);

    if (cmd->rangeCount > 0) {
        auto imageAspect = pRegions[0].aspectMask;
        auto fbo = mDummyFramebuffer.BindImage(mpStateManager, pImage, imageAspect, 1, &pRegions[0].baseMipLevel);
        mpStateManager->PushDrawFramebuffer(fbo);
        if (imageAspect == Shit::ImageAspectFlagBits(6)) {
            glClearDepth(cmd->depthStencil.depth);
            glClearStencil(cmd->depthStencil.stencil);
            glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        } else if (imageAspect == Shit::ImageAspectFlagBits::DEPTH_BIT) {
            glClearDepth(cmd->depthStencil.depth);
            glClear(GL_DEPTH_BUFFER_BIT);
        } else if (imageAspect == Shit::ImageAspectFlagBits::STENCIL_BIT) {
            glClearStencil(cmd->depthStencil.stencil);
            glClear(GL_STENCIL_BUFFER_BIT);
        }
        mpStateManager->PopDrawFramebuffer();
    }
    return offsetof(ClearDepthStencilImageInfo, pRanges) + sizeof(ImageSubresourceRange) * cmd->rangeCount;
}
size_t GLCommandBuffer::_ClearAttachments(const void *params) {
    auto cmd = reinterpret_cast<const ClearAttachmentsInfo *>(params);
    auto pAttachments = reinterpret_cast<const ClearAttachment *>(&cmd->attachmentCount + 1);
    auto pRectCount = reinterpret_cast<const uint32_t *>(pAttachments + cmd->attachmentCount);
    return sizeof(ClearAttachment) * cmd->attachmentCount + sizeof(ClearRect) * (*pRectCount) + sizeof(uint32_t) * 2;
}
size_t GLCommandBuffer::_CopyBuffer(const void *params) {
    auto cmd = reinterpret_cast<const CopyBufferInfo *>(params);
    mpStateManager->PushBuffer(GL_COPY_READ_BUFFER, static_cast<GLBuffer const *>(cmd->pSrcBuffer)->GetHandle());
    mpStateManager->PushBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLBuffer const *>(cmd->pDstBuffer)->GetHandle());
    auto regions = reinterpret_cast<const BufferCopy *>(&cmd->pRegions);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, static_cast<GLintptr>(regions[i].srcOffset),
                            static_cast<GLintptr>(regions[i].dstOffset), static_cast<GLsizeiptr>(regions[i].size));
    }
    mpStateManager->PopBuffer();
    mpStateManager->PopBuffer();
    return offsetof(CopyBufferInfo, pRegions) + sizeof(BufferCopy) * cmd->regionCount;
}
size_t GLCommandBuffer::_CopyBufferToImage(const void *params) {
    auto cmd = reinterpret_cast<const CopyBufferToImageInfo *>(params);
    mpStateManager->PushBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLBuffer const *>(cmd->pSrcBuffer)->GetHandle());
    auto regions = reinterpret_cast<const BufferImageCopy *>(&cmd->pRegions);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        glPixelStorei(GL_UNPACK_ROW_LENGTH, regions[i].bufferRowLength);
        glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, regions[i].bufferImageHeight);
        cmd->pDstImage->UpdateSubData(
            {}, {}, regions[i].imageSubresource.mipLevel,
            Rect3D{Offset3D{
                       regions[i].imageOffset.x,
                       static_cast<int32_t>(cmd->pDstImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_1D
                                                ? regions[i].imageSubresource.baseArrayLayer
                                                : regions[i].imageOffset.y),
                       static_cast<int32_t>(cmd->pDstImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_2D
                                                ? regions[i].imageSubresource.baseArrayLayer
                                                : regions[i].imageOffset.z),
                   },
                   Extent3D{
                       regions[i].imageExtent.width,
                       cmd->pDstImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_1D
                           ? regions[i].imageSubresource.layerCount
                           : regions[i].imageExtent.height,
                       cmd->pDstImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_2D
                           ? regions[i].imageSubresource.layerCount
                           : regions[i].imageExtent.depth,
                   }},
            {}, nullptr);
        //&regions[i].bufferOffset);
    }
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    mpStateManager->PopBuffer();
    return offsetof(CopyBufferToImageInfo, pRegions) + sizeof(BufferImageCopy) * cmd->regionCount;
}
size_t GLCommandBuffer::_CopyImage(const void *params) {
    auto cmd = reinterpret_cast<const CopyImageInfo *>(params);
    auto size = offsetof(CopyImageInfo, pRegions) + sizeof(ImageCopy) * cmd->regionCount;
    auto pSrcImage = static_cast<GLImage const *>(cmd->pSrcImage);
    auto pDstImage = static_cast<GLImage const *>(cmd->pDstImage);
    auto regions = reinterpret_cast<const ImageCopy *>(&cmd->pRegions);

    GLenum srcTarget, dstTarget;
    auto b = pDstImage->GetImageFlag();
    auto a = pSrcImage->GetImageFlag();

    if (a == GLImageFlag::TEXTURE)
        srcTarget = Map(cmd->pSrcImage->GetCreateInfoPtr()->imageType, cmd->pSrcImage->GetCreateInfoPtr()->samples);
    else if (a == GLImageFlag::RENDERBUFFER)
        srcTarget = GL_RENDERBUFFER;
    else {
        ST_LOG("copyImage not implemented yet");
        // TODO: use readpixel or blit to read framebuffer data
        return size;
    }
    if (b == GLImageFlag::TEXTURE)
        dstTarget = Map(cmd->pDstImage->GetCreateInfoPtr()->imageType, cmd->pDstImage->GetCreateInfoPtr()->samples);
    else if (b == GLImageFlag::RENDERBUFFER)
        dstTarget = GL_RENDERBUFFER;
    else {
        ST_LOG("copyImage not implemented yet");
        // TODO: use blit to copy to framebuffer
        return size;
    }
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        glCopyImageSubData(
            pSrcImage->GetHandle(), srcTarget, regions[i].srcSubresource.mipLevel, regions[i].srcOffset.x,
            regions[i].srcOffset.y,
            (std::max)(static_cast<uint32_t>(regions[i].srcOffset.z), regions[i].srcSubresource.baseArrayLayer),
            pDstImage->GetHandle(), dstTarget, regions[i].dstSubresource.mipLevel, regions[i].dstOffset.x,
            regions[i].dstOffset.y,
            (std::max)(static_cast<uint32_t>(regions[i].dstOffset.z), regions[i].dstSubresource.baseArrayLayer),
            regions[i].extent.width, regions[i].extent.height,
            (std::max)(regions[i].extent.depth, regions[i].srcSubresource.layerCount));
    }
    return size;
}
size_t GLCommandBuffer::_CopyImageToBuffer(const void *params) {
    auto cmd = reinterpret_cast<const CopyImageToBufferInfo *>(params);
    mpStateManager->PushBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLBuffer const *>(cmd->pDstBuffer)->GetHandle());
    // auto internalformat =
    // MapInternalFormat(cmd->pSrcImage->GetCreateInfoPtr()->format);
    auto externalformat = MapBaseFormat(cmd->pSrcImage->GetCreateInfoPtr()->format);

    auto type = Map(GetFormatDataType(cmd->pSrcImage->GetCreateInfoPtr()->format));

    auto a = static_cast<GLImage const *>(cmd->pSrcImage)->GetImageFlag();
    if (a == GLImageFlag::RENDERBUFFER) {
        // TODO:
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        mpStateManager->BindReadFramebuffer(fbo);
        glFramebufferRenderbuffer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
                                  static_cast<GLImage const *>(cmd->pSrcImage)->GetHandle());
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        auto regions = reinterpret_cast<const BufferImageCopy *>(&cmd->pRegions);
        for (uint32_t i = 0; i < cmd->regionCount; ++i) {
            glReadPixels(regions[i].imageOffset.x, regions[i].imageOffset.y, regions[i].imageExtent.width,
                         regions[i].imageExtent.height, externalformat, type,
                         (void *)(GLintptr)(regions[i]).bufferOffset);
        }
        mpStateManager->NotifyReleasedFramebuffer(fbo);
        glDeleteFramebuffers(1, &fbo);
    } else if (a == GLImageFlag::TEXTURE) {
        auto target = Map(cmd->pSrcImage->GetCreateInfoPtr()->imageType, cmd->pSrcImage->GetCreateInfoPtr()->samples);
        mpStateManager->BindTextureUnit(0, target, static_cast<GLImage const *>(cmd->pSrcImage)->GetHandle());
        auto regions = reinterpret_cast<const BufferImageCopy *>(&cmd->pRegions);
        for (uint32_t i = 0; i < cmd->regionCount; ++i) {
#if 0
					glGetTexImage(target,
								  regions[i].imageSubresource.mipLevel,
								  externalformat,
								  type,
								  0);
#endif
#if 1
            // opengl 4.5
            glGetTextureSubImage(
                static_cast<GLImage const *>(cmd->pSrcImage)->GetHandle(), regions[i].imageSubresource.mipLevel,
                regions[i].imageOffset.x,
                cmd->pSrcImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_1D
                    ? regions[i].imageSubresource.baseArrayLayer
                    : regions[i].imageOffset.y,
                cmd->pSrcImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_2D
                    ? regions[i].imageSubresource.baseArrayLayer
                    : regions[i].imageOffset.z,
                static_cast<GLsizei>(regions[i].imageExtent.width),
                static_cast<GLsizei>(cmd->pSrcImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_1D
                                         ? regions[i].imageSubresource.layerCount
                                         : regions[i].imageExtent.height),
                static_cast<GLsizei>(cmd->pSrcImage->GetCreateInfoPtr()->imageType == ImageType::TYPE_2D
                                         ? regions[i].imageSubresource.layerCount
                                         : regions[i].imageExtent.depth),
                externalformat, type, static_cast<GLsizei>(cmd->pDstBuffer->GetCreateInfoPtr()->size), 0);
#endif
        }
    } else {
        // TODO:copy default fbo to buffer
        ST_LOG("copy default fbo to buffer not implemented yet");
    }
    mpStateManager->PopBuffer();
    return offsetof(CopyImageToBufferInfo, pRegions) + sizeof(BufferImageCopy) * cmd->regionCount;
}
size_t GLCommandBuffer::_Dispatch(const void *params) {
    auto cmd = reinterpret_cast<const DispatchInfo *>(params);
    glDispatchCompute(cmd->groupCountX, cmd->groupCountY, cmd->groupCountZ);
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DispatchIndirect(const void *params) {
    auto cmd = reinterpret_cast<const DispatchIndirectInfo *>(params);
    mpStateManager->BindBuffer(GL_DISPATCH_INDIRECT_BUFFER, static_cast<const GLBuffer *>(cmd->pBuffer)->GetHandle());
    glDispatchComputeIndirect(static_cast<GLintptr>(cmd->offset));
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_Draw(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndirectCommand *>(params);
// opengl 4.2
#if 1
    glDrawArraysInstancedBaseInstance(mpStateManager->GetPrimitiveTopology(), cmd->firstVertex, cmd->vertexCount,
                                      cmd->instanceCount, cmd->firstInstance);
#else
    glDrawArraysInstanced(mpStateManager->GetPrimitiveTopology(), cmd->firstVertex, cmd->vertexCount,
                          cmd->instanceCount);
#endif
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DrawIndirect(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndirectInfo *>(params);
    mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer const *>(cmd->pBuffer)->GetHandle());
    // opengl 4.3
#if 1
    glMultiDrawArraysIndirect(mpStateManager->GetPrimitiveTopology(), (void *)(GLintptr)(cmd->offset), cmd->drawCount,
                              cmd->stride);
#else
    for (uint32_t i = 0; i < cmd->drawCount; ++i) {
        // offset += i * sizeof(DrawIndirectCommand);
        glDrawArraysIndirect(mpStateManager->GetPrimitiveTopology(), (void *)&offset[i]);
    }
#endif
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DrawIndirectCount(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(params);
    mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer const *>(cmd->pBuffer)->GetHandle());
    mpStateManager->PushBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer const *>(cmd->pCountBuffer)->GetHandle());
    // opengl 4.6
    glMultiDrawArraysIndirectCount(mpStateManager->GetPrimitiveTopology(), nullptr,
                                   static_cast<GLintptr>(cmd->countBufferOffset), cmd->maxDrawCount, cmd->stride);
    mpStateManager->PopBuffer();
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DrawIndexed(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndexedIndirectCommand *>(params);
    auto indexTypeSize = 2;
    switch (mpStateManager->GetIndexType()) {
        case GL_UNSIGNED_BYTE:
            indexTypeSize = 1;
            break;
        case GL_UNSIGNED_SHORT:
        default:
            indexTypeSize = 2;
            break;
        case GL_UNSIGNED_INT:
            indexTypeSize = 4;
            break;
    }
    auto offset = cmd->firstIndex * indexTypeSize;
    // TODO: check opengl version
#if 1
    // opengl 4.2
    glDrawElementsInstancedBaseVertexBaseInstance(mpStateManager->GetPrimitiveTopology(), cmd->indexCount,
                                                  mpStateManager->GetIndexType(), (void *)(GLintptr)(offset),
                                                  cmd->instanceCount, cmd->vertexOffset, cmd->firstInstance);
#else
    glDrawElementsInstanced(mpStateManager->GetPrimitiveTopology(), cmd->indexCount, mpStateManager->GetIndexType(),
                            (void *)(GLintptr)(offset), cmd->instanceCount);
#endif
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DrawIndexedIndirect(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndirectInfo *>(params);
    mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer const *>(cmd->pBuffer)->GetHandle());
    // opengl4.3
    glMultiDrawElementsIndirect(mpStateManager->GetPrimitiveTopology(), mpStateManager->GetIndexType(),
                                (void *)(GLintptr)(cmd->offset),  // offset of drawcount
                                                                  // drawelementsindirectcommand buffer
                                cmd->drawCount, cmd->stride);
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_DrawIndexedIndirectCount(const void *params) {
    auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(params);
    mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer const *>(cmd->pBuffer)->GetHandle());
    mpStateManager->PushBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer const *>(cmd->pCountBuffer)->GetHandle());

    // opengl 4.6
    glMultiDrawElementsIndirectCount(mpStateManager->GetPrimitiveTopology(), mpStateManager->GetIndexType(),
                                     (void *)(GLintptr)cmd->offset, static_cast<GLintptr>(cmd->countBufferOffset),
                                     cmd->maxDrawCount, cmd->stride);
    mpStateManager->PopBuffer();
#ifndef NDEBUG
    mpStateManager->CheckError();
#endif
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_EndRenderPass(ST_MAYBE_UNUSED const void *params) {
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
    glTextureBarrier();
    if (mpCurFramebuffer) mpCurFramebuffer->Resolve(mCurSubpass, Filter::LINEAR);
    mpCurFramebuffer = nullptr;
    mCurRenderPass = nullptr;
    mCurSubpass = 0;
    return 0;
}
size_t GLCommandBuffer::_EndTransformFeedback(const void *params) {
    auto cmd = reinterpret_cast<const EndTransformFeedbackInfo *>(params);
    glEndTransformFeedback();
    return sizeof(uint32_t) * 2 + (sizeof(ptrdiff_t) + sizeof(uint64_t)) * cmd->counterBufferCount;
}
size_t GLCommandBuffer::_FillBuffer(const void *params) {
    auto cmd = reinterpret_cast<const FillBufferInfo *>(params);
    auto buffer = static_cast<const GLBuffer *>(cmd->buffer);
    mpStateManager->BindBuffer(GL_ARRAY_BUFFER, buffer->GetHandle());
    auto size = cmd->size;
    if (size == ST_WHOLE_SIZE) size = buffer->GetCreateInfoPtr()->size - cmd->offset;
    std::vector<uint32_t> data(size / 4, cmd->data);
    glClearBufferSubData(GL_ARRAY_BUFFER, GL_R32UI, cmd->offset, size, GL_RED_INTEGER, GL_UNSIGNED_INT,
                         (const void *)data.data());
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_GenerateMipmap(const void *params) {
    auto cmd = reinterpret_cast<const GenerateMipmapInfo *>(params);
    cmd->pImage->GenerateMipmap(cmd->filter, cmd->srcImageLayout, cmd->dstImageLayout);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_UpdateBuffer(const void *params) {
    auto cmd = reinterpret_cast<const UpdateBufferInfo *>(params);
    return offsetof(UpdateBufferInfo, pData) + cmd->dataSize;
}
size_t GLCommandBuffer::_PipelineBarrier(const void *params) {
    // TODO: how to set memory barrier??
    auto cmd = reinterpret_cast<const PipelineBarrierInfo *>(params);
    auto p = reinterpret_cast<const char *>(&cmd->pMemoryBarriers);
    //[[maybe_unused]] auto pMemoryBarrier = reinterpret_cast<const MemoryBarrier
    //*>(p);
    // for (uint32_t i = 0; i < cmd->memoryBarrierCount; ++i)
    //{
    //}
    p += sizeof(MemoryBarrier) * cmd->memoryBarrierCount;
    if (cmd->memoryBarrierCount) {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    auto bufferMemoryBarrierCount = *reinterpret_cast<const uint32_t *>(p);
    if (bufferMemoryBarrierCount) {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    p += sizeof(uint32_t);
    // auto pbuffermemorybarriers = reinterpret_cast<BufferMemoryBarrier *>(p);
    //  for (uint32_t i = 0; i < buffermemorybarriercount; ++i)
    //{
    // }
    p += sizeof(BufferMemoryBarrier) * bufferMemoryBarrierCount;
    auto imageMemoryBarrierCount = *reinterpret_cast<const uint32_t *>(p);
    p += sizeof(uint32_t);
    //[[maybe_unused]] auto pImageMemoryBarriers =
    // reinterpret_cast<ImageMemoryBarrier *>(p);
    // for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i)
    //{
    //}
    if (imageMemoryBarrierCount) {
        glTextureBarrier();
    }
    return offsetof(PipelineBarrierInfo, pMemoryBarriers) + +sizeof(uint32_t) * 2 +
           sizeof(MemoryBarrier) * cmd->memoryBarrierCount + sizeof(BufferMemoryBarrier) * bufferMemoryBarrierCount +
           sizeof(ImageMemoryBarrier) * imageMemoryBarrierCount;
}
size_t GLCommandBuffer::_PushConstants(const void *params) {
    // TODO: how to optimize
    auto cmd = reinterpret_cast<const PushConstantInfo *>(params);
    dynamic_cast<GLPipelineLayout const *>(cmd->pPipelineLayout)
        ->PushConstant(cmd->stageFlags, cmd->offset, cmd->size, &cmd->pValues);
    return offsetof(PushConstantInfo, pValues) + cmd->size;
}
size_t GLCommandBuffer::_ResolveImage(const void *params) {
    auto cmd = reinterpret_cast<const ResolveImageInfo *>(params);
    auto regions = reinterpret_cast<const ImageResolve *>(&cmd->pRegions);
    std::vector<uint32_t> srcLevels(cmd->regionCount);
    std::vector<uint32_t> dstLevels(cmd->regionCount);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        srcLevels[i] = regions[i].srcSubresource.mipLevel;
        dstLevels[i] = regions[i].dstSubresource.mipLevel;
    }
    auto srcFBO = mDummyFramebuffer.BindImage(mpStateManager, static_cast<GLImage const *>(cmd->pSrcImage),
                                              ImageAspectFlagBits::COLOR_BIT, cmd->regionCount, srcLevels.data());
    auto dstFBO = mDummyFramebuffer2.BindImage(mpStateManager, static_cast<GLImage const *>(cmd->pDstImage),
                                               ImageAspectFlagBits::COLOR_BIT, cmd->regionCount, dstLevels.data());
    mpStateManager->PushReadFramebuffer(srcFBO);
    mpStateManager->PushDrawFramebuffer(dstFBO);
    for (uint32_t i = 0; i < cmd->regionCount; ++i) {
        glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        auto &&e = cmd->pRegions[i];
        GLenum mask = 0;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::COLOR_BIT)) mask |= GL_COLOR_BUFFER_BIT;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::DEPTH_BIT)) mask |= GL_DEPTH_BUFFER_BIT;
        if (bool(e.srcSubresource.aspectMask & ImageAspectFlagBits::STENCIL_BIT)) mask |= GL_STENCIL_BUFFER_BIT;
        glBlitFramebuffer(e.srcOffset.x, e.srcOffset.y, e.extent.width - e.srcOffset.x, e.extent.height - e.srcOffset.y,
                          e.dstOffset.x, e.dstOffset.y, e.extent.width - e.dstOffset.x, e.extent.height - e.dstOffset.y,
                          mask, GL_LINEAR);
    }
    mpStateManager->PopReadFramebuffer();
    mpStateManager->PopDrawFramebuffer();
    return offsetof(ResolveImageInfo, pRegions) + sizeof(ImageResolve) * cmd->regionCount;
}
size_t GLCommandBuffer::_ExecuteCommands(const void *params) {
    auto cmd = reinterpret_cast<const ExecuteCommandsInfo *>(params);
    auto pCommandBuffers =
        const_cast<CommandBuffer **>(reinterpret_cast<CommandBuffer const *const *>(&cmd->pCommandBuffers));
    for (uint32_t i = 0; i < cmd->count; ++i) {
        static_cast<GLCommandBuffer *>(pCommandBuffers[i])->Execute();
    }
    return offsetof(ExecuteCommandsInfo, pCommandBuffers) + sizeof(ptrdiff_t) * cmd->count;
}
size_t GLCommandBuffer::_SetBlendConstants(const void *) {
    // auto cmd = reinterpret_cast<const float *>(params);
    return sizeof(float) * 4;
}
size_t GLCommandBuffer::_SetDepthBias(const void *params) {
    auto cmd = reinterpret_cast<const SetDepthBiasInfo *>(params);
    mpStateManager->PolygonOffSetClamp(cmd->depthBiasSlopFactor, cmd->depthBiasConstantFactor, cmd->depthBiasClamp);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_SetDepthBounds(const void *params) {
    auto cmd = reinterpret_cast<const SetDepthBoundsInfo *>(params);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_SetLineWidth(const void *params) {
    auto cmd = *reinterpret_cast<const float *>(params);
    mpStateManager->LineWidth(cmd);
    return sizeof(cmd);
}
size_t GLCommandBuffer::_SetStencilCompareMask(const void *params) {
    auto cmd = reinterpret_cast<const SetStencilCompareMaskInfo *>(params);
    if (static_cast<bool>(cmd->faceMask & StencilFaceFlagBits::FRONT_BIT))
        mpStateManager->StencilCompareMask(GL_FRONT, cmd->mask);
    if (static_cast<bool>(cmd->faceMask & StencilFaceFlagBits::BACK_BIT))
        mpStateManager->StencilCompareMask(GL_BACK, cmd->mask);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_SetStencilReference(const void *params) {
    auto cmd = reinterpret_cast<const SetStencilReferenceInfo *>(params);
    if (static_cast<bool>(cmd->faceMask & StencilFaceFlagBits::FRONT_BIT))
        mpStateManager->StencilReference(GL_FRONT, cmd->mask);
    if (static_cast<bool>(cmd->faceMask & StencilFaceFlagBits::BACK_BIT))
        mpStateManager->StencilReference(GL_BACK, cmd->mask);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_SetStencilWriteMask(const void *params) {
    auto cmd = reinterpret_cast<const SetStencilWriteMaskInfo *>(params);
    return sizeof(*cmd);
}
size_t GLCommandBuffer::_SetScissor(const void *params) {
    auto cmd = reinterpret_cast<const SetScissorInfo *>(params);
    mpStateManager->SetScissors(cmd->firstScissor, cmd->scissorCount,
                                reinterpret_cast<const int32_t *>(&cmd->pScissors));
    return offsetof(SetScissorInfo, pScissors) + sizeof(Rect2D) * cmd->scissorCount;
}
size_t GLCommandBuffer::_SetViewport(const void *params) {
    auto cmd = reinterpret_cast<const SetViewPortInfo *>(params);
    mpStateManager->SetViewports(cmd->firstViewport, cmd->viewportCount,
                                 reinterpret_cast<const float *>(&cmd->pViewports));
    return offsetof(SetViewPortInfo, pViewports) + sizeof(Viewport) * cmd->viewportCount;
}
size_t GLCommandBuffer::_NextSubpass(const void *params) {
    auto cmd = reinterpret_cast<const SubpassContents *>(params);
    if (mpCurFramebuffer) {
        // subpass barrier
        glTextureBarrier();
        mpCurFramebuffer->Resolve(mCurSubpass, Filter::LINEAR);
        mpCurFramebuffer->BindFBO(mCurSubpass + 1, true, true);
    }
    ++mCurSubpass;
    return sizeof(*cmd);
}
void GLCommandBuffer::Execute() {
    auto pCur = mBuffer.data();
    auto pEnd = pCur + mBuffer.size();
    GLCommandCode cmdCode;
    while (pCur < pEnd) {
        cmdCode = *(reinterpret_cast<const GLCommandCode *>(pCur));
        pCur += sizeof(GLCommandCode);
        auto func = mCommandFunctions[static_cast<size_t>(cmdCode)];
        if (func)
            pCur += std::invoke(func, this, pCur);
        else
            ST_THROW("command function", (int)cmdCode, "not implemented yet");
    }
}
template <class T>
T *GLCommandBuffer::AllocateCommand(GLCommandCode commandCode, size_t realsize) {
    auto offset = mBuffer.size();
    if (realsize == 0)
        mBuffer.insert(mBuffer.end(), sizeof(GLCommandCode) + sizeof(T), {});
    else
        mBuffer.insert(mBuffer.end(), sizeof(GLCommandCode) + realsize, {});
    mBuffer[offset] = static_cast<uint8_t>(commandCode);
    return reinterpret_cast<T *>(&mBuffer[offset + sizeof(GLCommandCode)]);
}
template <>
void *GLCommandBuffer::AllocateCommand<void>(GLCommandCode commandCode, size_t realsize) {
    auto offset = mBuffer.size();
    mBuffer.insert(mBuffer.end(), sizeof(GLCommandCode) + realsize, {});
    mBuffer[offset] = static_cast<uint8_t>(commandCode);
    return realsize == 0 ? &mBuffer[offset] : &mBuffer[offset + sizeof(GLCommandCode)];
}
void GLCommandBuffer::ExecuteCommands(const ExecuteCommandsInfo &info) {
    auto p = AllocateCommand<ExecuteCommandsInfo>(
        GLCommandCode::EXECUTE_COMMANDS,
        offsetof(ExecuteCommandsInfo, pCommandBuffers) + sizeof(ptrdiff_t) * info.count);
    p->count = info.count;
    memcpy(&p->pCommandBuffers, info.pCommandBuffers, sizeof(ptrdiff_t) * info.count);
}
void GLCommandBuffer::Reset(CommandBufferResetFlatBits) { mBuffer.clear(); }
void GLCommandBuffer::Begin(const CommandBufferBeginInfo &beginInfo) {
    if (static_cast<bool>(beginInfo.usage & CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT)) mBuffer.clear();

    if (beginInfo.inheritanceInfo.pRenderPass) {
        mCurRenderPass = beginInfo.inheritanceInfo.pRenderPass;
        mCurSubpass = beginInfo.inheritanceInfo.subpass;
        mpCurFramebuffer =
            const_cast<GLFramebuffer *>(static_cast<GLFramebuffer const *>(beginInfo.inheritanceInfo.pFramebuffer));
    }
}
void GLCommandBuffer::End() {}
void GLCommandBuffer::BeginRenderPass(const BeginRenderPassInfo &beginInfo) {
    for (size_t i = 0; i < beginInfo.clearValueCount; ++i)
        const_cast<GLRenderPass *>(static_cast<GLRenderPass const *>(beginInfo.pRenderPass))
            ->SetAttachmentClearValue(i, beginInfo.pClearValues[i]);

    auto p = AllocateCommand<BeginRenderPassInfo>(GLCommandCode::BEGIN_RENDERPASS,
                                                  offsetof(BeginRenderPassInfo, clearValueCount));
    memcpy(p, &beginInfo, offsetof(BeginRenderPassInfo, clearValueCount));
}
void GLCommandBuffer::EndRenderPass() { AllocateCommand<void>(GLCommandCode::END_RENDERPASS); }
void GLCommandBuffer::NextSubpass(SubpassContents subpassContents) {
    memcpy(AllocateCommand<SubpassContents>(GLCommandCode::NEXT_SUBPASS), &subpassContents, sizeof(SubpassContents));
}
void GLCommandBuffer::BindPipeline(const BindPipelineInfo &info) {
    memcpy(AllocateCommand<BindPipelineInfo>(GLCommandCode::BIND_PIPELINE), &info, sizeof(BindPipelineInfo));
}
void GLCommandBuffer::CopyBuffer(const CopyBufferInfo &copyInfo) {
    auto p = AllocateCommand<CopyBufferInfo>(
        GLCommandCode::COPY_BUFFER, offsetof(CopyBufferInfo, pRegions) + sizeof(BufferCopy) * copyInfo.regionCount);
    memcpy(p, &copyInfo, offsetof(CopyBufferInfo, pRegions));
    memcpy(&p->pRegions, copyInfo.pRegions, sizeof(BufferCopy) * copyInfo.regionCount);
}
void GLCommandBuffer::CopyImage(const CopyImageInfo &copyInfo) {
    auto p = AllocateCommand<CopyImageInfo>(
        GLCommandCode::COPY_IMAGE, offsetof(CopyImageInfo, pRegions) + sizeof(ImageCopy) * copyInfo.regionCount);
    memcpy(p, &copyInfo, offsetof(CopyImageInfo, pRegions));
    memcpy(&p->pRegions, copyInfo.pRegions, sizeof(ImageCopy) * copyInfo.regionCount);
}
void GLCommandBuffer::CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) {
    auto p = AllocateCommand<CopyBufferToImageInfo>(
        GLCommandCode::COPY_BUFFER_TO_IMAGE,
        offsetof(CopyBufferToImageInfo, pRegions) + copyInfo.regionCount * sizeof(BufferImageCopy));
    memcpy(p, &copyInfo, offsetof(CopyBufferToImageInfo, pRegions));
    memcpy(&p->pRegions, copyInfo.pRegions, sizeof(BufferImageCopy) * copyInfo.regionCount);
}
void GLCommandBuffer::CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) {
    auto p = AllocateCommand<CopyImageToBufferInfo>(
        GLCommandCode::COPY_IMAGE_TO_BUFFER,
        offsetof(CopyImageToBufferInfo, pRegions) + copyInfo.regionCount * sizeof(BufferImageCopy));
    memcpy(p, &copyInfo, offsetof(CopyImageToBufferInfo, pRegions));
    memcpy(&p->pRegions, copyInfo.pRegions, sizeof(BufferImageCopy) * copyInfo.regionCount);
}
void GLCommandBuffer::BlitImage(const BlitImageInfo &blitInfo) {
    auto p = AllocateCommand<BlitImageInfo>(
        GLCommandCode::BLIT_IMAGE, offsetof(BlitImageInfo, pRegions) + sizeof(ImageBlit) * blitInfo.regionCount);
    memcpy(p, &blitInfo, offsetof(BlitImageInfo, pRegions));
    memcpy(&p->pRegions, blitInfo.pRegions, sizeof(ImageBlit) * blitInfo.regionCount);
}
void GLCommandBuffer::BindVertexBuffers(const BindVertexBuffersInfo &info) {
    auto p = AllocateCommand<BindVertexBuffersInfo>(
        GLCommandCode::BIND_VERTEX_BUFFER,
        offsetof(BindVertexBuffersInfo, ppBuffers) + (sizeof(ptrdiff_t) + sizeof(uint64_t)) * info.bindingCount);
    memcpy(p, &info, offsetof(BindVertexBuffersInfo, ppBuffers));
    memcpy(&p->ppBuffers, info.ppBuffers, info.bindingCount * sizeof(ptrdiff_t));
    memcpy(&p->ppBuffers + info.bindingCount, info.pOffsets, info.bindingCount * sizeof(uint64_t));
}
void GLCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info) {
    memcpy(AllocateCommand<BindIndexBufferInfo>(GLCommandCode::BIND_INDEXBUFFER), &info, sizeof(BindIndexBufferInfo));
    mCurIndexType = info.indexType;
    mCurIndexOffset = info.offset;
    if (info.offset > 0) {
        ST_LOG("current gl donot support indexbuffer offset!!!")
    }
}
void GLCommandBuffer::BindDescriptorSets(const BindDescriptorSetsInfo &info) {
    auto p = AllocateCommand<BindDescriptorSetsInfo>(GLCommandCode::BIND_DESCRIPTORSETS,
                                                     offsetof(BindDescriptorSetsInfo, ppDescriptorSets) +
                                                         sizeof(ptrdiff_t) * info.descriptorSetCount +
                                                         +sizeof(uint32_t) * (size_t)(1 + info.dynamicOffsetCount));
    memcpy(p, &info, offsetof(BindDescriptorSetsInfo, ppDescriptorSets));
    memcpy(&p->ppDescriptorSets, info.ppDescriptorSets, sizeof(ptrdiff_t) * info.descriptorSetCount);
    memcpy(&p->ppDescriptorSets + info.descriptorSetCount, &info.dynamicOffsetCount, sizeof(uint32_t));
    auto p2 = reinterpret_cast<uint32_t *>(&p->ppDescriptorSets + info.descriptorSetCount) + 1;
    memcpy(p2, &info.pDynamicOffsets, sizeof(uint32_t) * info.dynamicOffsetCount);
}
void GLCommandBuffer::Draw(const DrawIndirectCommand &info) {
    memcpy(AllocateCommand<DrawIndirectCommand>(GLCommandCode::DRAW), &info, sizeof(DrawIndirectCommand));
}
void GLCommandBuffer::Draw(const DrawIndexedIndirectCommand &info) { DrawIndexed(info); }
void GLCommandBuffer::DrawIndirect(const DrawIndirectInfo &info) {
    memcpy(AllocateCommand<DrawIndirectInfo>(GLCommandCode::DRAW_INDIRECT), &info, sizeof(DrawIndirectInfo));
}
void GLCommandBuffer::DrawIndirect(const DrawIndirectCountInfo &info) { DrawIndirectCount(info); }
void GLCommandBuffer::DrawIndirectCount(const DrawIndirectCountInfo &info) {
    memcpy(AllocateCommand<DrawIndirectCountInfo>(GLCommandCode::DRAW_INDIRECT_COUNT), &info,
           sizeof(DrawIndirectCountInfo));
}
void GLCommandBuffer::DrawIndexed(const DrawIndexedIndirectCommand &info) {
    memcpy(AllocateCommand<DrawIndexedIndirectCommand>(GLCommandCode::DRAW_INDEXED), &info,
           sizeof(DrawIndexedIndirectCommand));
}
void GLCommandBuffer::DrawIndexedIndirect(const DrawIndirectInfo &info) {
    // if (mCurIndexOffset > 0)
    //{
    //	// TODO: will repeat some times, how to fix? DO NOT use gl command here
    //	static GLuint stageBuffer{};
    //	if (!stageBuffer)
    //	{
    //		glGenBuffers(1, &stageBuffer);
    //		mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, stageBuffer);
    //		glBufferStorage(GL_COPY_WRITE_BUFFER,
    // sizeof(DrawIndexedIndirectCommand), nullptr, GL_MAP_READ_BIT |
    // GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
    //	}
    //	mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, stageBuffer);
    //	mpStateManager->BindBuffer(GL_COPY_READ_BUFFER, static_cast<GLBuffer
    // const *>(info.pBuffer)->GetHandle());
    //	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
    // sizeof(DrawIndexedIndirectCommand));

    //	auto p = (DrawIndexedIndirectCommand
    //*)glMapBufferRange(GL_COPY_WRITE_BUFFER, 0,
    // sizeof(DrawIndexedIndirectCommand),
    // GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);

    //	p->firstIndex = static_cast<uint32_t>(mCurIndexOffset) /
    // GetIndexTypeSize(mCurIndexType); 	glUnmapBuffer(GL_COPY_WRITE_BUFFER);

    //	mpStateManager->BindBuffer(GL_COPY_READ_BUFFER, stageBuffer);
    //	mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLBuffer
    // const *>(info.pBuffer)->GetHandle());
    //	glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0,
    // sizeof(DrawIndexedIndirectCommand));

    //	// glDeleteBuffers(1, &stageBuffer);
    //	// mpStateManager->NotifyReleaseBuffer(stageBuffer);
    //}
    memcpy(AllocateCommand<DrawIndirectInfo>(GLCommandCode::DRAW_INDEXED_INDIRECT), &info, sizeof(DrawIndirectInfo));
}
void GLCommandBuffer::DrawIndexedIndirect(const DrawIndirectCountInfo &info) { DrawIndexedIndirectCount(info); }
void GLCommandBuffer::DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) {
    memcpy(AllocateCommand<DrawIndirectCountInfo>(GLCommandCode::DRAW_INDEXED_INDIRECT_COUNT), &info,
           sizeof(DrawIndirectCountInfo));
}
void GLCommandBuffer::ClearColorImage(const ClearColorImageInfo &info) {
    auto p = AllocateCommand<ClearColorImageInfo>(
        GLCommandCode::CLEAR_COLOR_IMAGE,
        offsetof(ClearColorImageInfo, pRanges) + sizeof(ImageSubresourceRange) * info.rangeCount);
    memcpy(p, &info, offsetof(ClearColorImageInfo, pRanges));
    memcpy(&p->pRanges, info.pRanges, sizeof(ImageSubresourceRange) * info.rangeCount);
}
void GLCommandBuffer::ClearDepthStencilImage(const ClearDepthStencilImageInfo &info) {
    auto p = AllocateCommand<ClearDepthStencilImageInfo>(
        GLCommandCode::CLEAR_DEPTH_STENCIL_IMAGE,
        offsetof(ClearDepthStencilImageInfo, pRanges) + sizeof(ImageSubresourceRange) * info.rangeCount);
    memcpy(p, &info, sizeof(ClearDepthStencilImageInfo));
    memcpy(&p->pRanges, info.pRanges, sizeof(ImageSubresourceRange) * info.rangeCount);
}
void GLCommandBuffer::ClearAttachments(const ClearAttachmentsInfo &info) {
    auto p = AllocateCommand<ClearAttachmentsInfo>(
        GLCommandCode::CLEAR_ATTACHMENTS,
        sizeof(ClearAttachment) * info.attachmentCount + sizeof(ClearRect) * info.rectCount + sizeof(uint32_t) * 2);
    p->attachmentCount = info.attachmentCount;

    char *p2 = reinterpret_cast<char *>(&p->attachmentCount + 1);
    auto s = sizeof(ClearAttachment) * info.attachmentCount;
    memcpy(p2, info.pAttachments, s);

    p2 += s;
    s = sizeof(uint32_t);
    memcpy(p2, &info.rectCount, s);

    p2 += s;
    s = sizeof(ClearRect) * info.rectCount;
    memcpy(p2, info.pRects, s);
}
void GLCommandBuffer::FillBuffer(const FillBufferInfo &info) {
    memcpy(AllocateCommand<FillBufferInfo>(GLCommandCode::FILL_BUFFER), &info, sizeof(FillBufferInfo));
}
void GLCommandBuffer::UpdateBuffer(const UpdateBufferInfo &info) {
    auto p = reinterpret_cast<char *>(AllocateCommand<UpdateBufferInfo>(
        GLCommandCode::UPDATE_BUFFER, offsetof(UpdateBufferInfo, pData) + info.dataSize));
    auto s = offsetof(UpdateBufferInfo, pData);
    memcpy(p, &info, s);
    p += s;
    s = info.dataSize;
    memcpy(p, info.pData, s);
}
void GLCommandBuffer::PipelineBarrier(const PipelineBarrierInfo &info) {
    auto p = AllocateCommand<PipelineBarrierInfo>(
        GLCommandCode::PIPELINE_BARRIER, offsetof(PipelineBarrierInfo, pMemoryBarriers) + sizeof(uint32_t) * 2 +
                                             sizeof(MemoryBarrier) * info.memoryBarrierCount +
                                             sizeof(BufferMemoryBarrier) * info.bufferMemoryBarrierCount +
                                             sizeof(ImageMemoryBarrier) * info.imageMemoryBarrierCount);
    memcpy(p, &info, offsetof(PipelineBarrierInfo, pMemoryBarriers));

    char *p2 = reinterpret_cast<char *>(&p->pMemoryBarriers);
    memcpy(p2, info.pMemoryBarriers, sizeof(MemoryBarrier) * info.memoryBarrierCount);
    p2 += sizeof(MemoryBarrier) * info.memoryBarrierCount;
    memcpy(p2, &info.bufferMemoryBarrierCount, sizeof(uint32_t));
    p2 += sizeof(uint32_t);
    memcpy(p2, info.pBufferMemoryBarriers, sizeof(BufferMemoryBarrier) * info.bufferMemoryBarrierCount);
    p2 += sizeof(BufferMemoryBarrier) * info.bufferMemoryBarrierCount;
    memcpy(p2, &info.imageMemoryBarrierCount, sizeof(uint32_t));
    p2 += sizeof(uint32_t);
    memcpy(p2, info.pImageMemoryBarriers, sizeof(ImageMemoryBarrier) * info.imageMemoryBarrierCount);
}
void GLCommandBuffer::PushConstants(const PushConstantInfo &info) {
    auto p = AllocateCommand<PushConstantInfo>(GLCommandCode::PUSH_CONSTANTS,
                                               offsetof(PushConstantInfo, pValues) + info.size);
    memcpy(p, &info, offsetof(PushConstantInfo, pValues));
    memcpy(&p->pValues, info.pValues, info.size);
}
void GLCommandBuffer::PushDescriptorSet(const PushDescriptorSetInfo &) {
    // auto p = AllocateCommand<PushConstantInfo>(
    //	GLCommandCode::PUSH_CONSTANTS, offsetof(PushConstantInfo, pValues) +
    // info.size);
    ST_THROW("PushDescriptorSet not implemented yet")
}
void GLCommandBuffer::Dispatch(const DispatchInfo &info) {
    memcpy(AllocateCommand<DispatchInfo>(GLCommandCode::DISPATCH), &info, sizeof(info));
}
void GLCommandBuffer::DispatchIndirect(const DispatchIndirectInfo &info) {
    memcpy(AllocateCommand<DispatchIndirectInfo>(GLCommandCode::DISPATCH_INDIRECT), &info, sizeof(info));
}
void GLCommandBuffer::BindTransformFeedbackBuffers(const BindTransformFeedbackBuffersInfo &info) {
    auto p = AllocateCommand<BindTransformFeedbackBuffersInfo>(
        GLCommandCode::BIND_TRANSFORMFEEDBACK_BUFFERS,
        offsetof(BindTransformFeedbackBuffersInfo, ppBuffers) +
            (sizeof(ptrdiff_t) + sizeof(uint64_t) * 2) * info.bindingCount);
    memcpy(p, &info, offsetof(BindTransformFeedbackBuffersInfo, ppBuffers));
    char *p2 = reinterpret_cast<char *>(&p->ppBuffers);
    memcpy(p2, info.ppBuffers, sizeof(ptrdiff_t) * info.bindingCount);
    p2 += sizeof(ptrdiff_t) * info.bindingCount;
    memcpy(p2, info.pOffsets, sizeof(uint64_t) * info.bindingCount);
    p2 += sizeof(uint64_t) * info.bindingCount;
    memcpy(p2, info.pSizes, sizeof(uint64_t) * info.bindingCount);
}
void GLCommandBuffer::BeginTransformFeedback(const BeginTransformFeedbackInfo &info) {
    auto p = AllocateCommand<BeginTransformFeedbackInfo>(
        GLCommandCode::BEGIN_TRANSFORMFEEDBACK, offsetof(BeginTransformFeedbackInfo, ppCounterBuffers) +
                                                    (sizeof(ptrdiff_t) + sizeof(uint64_t)) * info.counterBufferCount);
    memcpy(p, &info, offsetof(BeginTransformFeedbackInfo, ppCounterBuffers));
    char *p2 = reinterpret_cast<char *>(&p->ppCounterBuffers);
    memcpy(p2, info.ppCounterBuffers, sizeof(ptrdiff_t) * info.counterBufferCount);
    p2 += sizeof(ptrdiff_t) * info.counterBufferCount;
    memcpy(p2, info.pCounterBufferOffsets, sizeof(uint64_t) * info.counterBufferCount);
}
void GLCommandBuffer::EndTransformFeedback(const EndTransformFeedbackInfo &info) {
    auto p = AllocateCommand<EndTransformFeedbackInfo>(
        GLCommandCode::END_TRANSFORMFEEDBACK, offsetof(EndTransformFeedbackInfo, ppCounterBuffers) +
                                                  (sizeof(ptrdiff_t) + sizeof(uint64_t)) * info.counterBufferCount);
    memcpy(p, &info, offsetof(EndTransformFeedbackInfo, ppCounterBuffers));
    char *p2 = reinterpret_cast<char *>(&p->ppCounterBuffers);
    memcpy(p2, info.ppCounterBuffers, sizeof(ptrdiff_t) * info.counterBufferCount);
    p2 += sizeof(ptrdiff_t) * info.counterBufferCount;
    memcpy(p2, info.pCounterBufferOffsets, sizeof(uint64_t) * info.counterBufferCount);
}
void GLCommandBuffer::SetViewport(const SetViewPortInfo &info) {
    auto p = AllocateCommand<SetViewPortInfo>(
        GLCommandCode::SET_VIEWPORT, offsetof(SetViewPortInfo, pViewports) + sizeof(Viewport) * info.viewportCount);
    memcpy(p, &info, offsetof(SetViewPortInfo, pViewports));
    memcpy(&p->pViewports, info.pViewports, sizeof(Viewport) * info.viewportCount);
}
void GLCommandBuffer::SetLineWidth(float lineWidth) {
    auto p = AllocateCommand<float>(GLCommandCode::SET_LINE_WIDTH);
    *p = lineWidth;
}
void GLCommandBuffer::SetDepthBias(const SetDepthBiasInfo &info) {
    auto p = AllocateCommand<SetDepthBiasInfo>(GLCommandCode::SET_DEPTH_BIAS);
    *p = info;
}
void GLCommandBuffer::SetBlendConstants(const float blendConstants[4]) {
    auto p = AllocateCommand<float>(GLCommandCode::SET_BLEND_CONSTANTS, sizeof(float) * 4);
    memcpy(p, blendConstants, sizeof(float) * 4);
}
void GLCommandBuffer::SetScissor(const SetScissorInfo &info) {
    auto p = AllocateCommand<SetScissorInfo>(GLCommandCode::SET_SCISSOR,
                                             offsetof(SetScissorInfo, pScissors) + sizeof(Rect2D) * info.scissorCount);
    memcpy(p, &info, offsetof(SetScissorInfo, pScissors));
    memcpy(&p->pScissors, info.pScissors, sizeof(Rect2D) * info.scissorCount);
}
void GLCommandBuffer::SetDepthBounds(const SetDepthBoundsInfo &info) {
    auto p = AllocateCommand<SetDepthBoundsInfo>(GLCommandCode::SET_DEPTH_BOUNDS);
    *p = info;
}
void GLCommandBuffer::SetStencilCompareMask(const SetStencilCompareMaskInfo &info) {
    auto p = AllocateCommand<SetStencilCompareMaskInfo>(GLCommandCode::SET_STENCIL_COMPARE_MASK);
    *p = info;
}
void GLCommandBuffer::SetStencilWriteMask(const SetStencilWriteMaskInfo &info) {
    auto p = AllocateCommand<SetStencilWriteMaskInfo>(GLCommandCode::SET_STENCIL_WRITE_MASK);
    *p = info;
}
void GLCommandBuffer::SetStencilReference(const SetStencilReferenceInfo &info) {
    auto p = AllocateCommand<SetStencilReferenceInfo>(GLCommandCode::SET_STENCIL_REFERENCE);
    *p = info;
}
void GLCommandBuffer::ResolveImage(const ResolveImageInfo &info) {
    auto p = AllocateCommand<ResolveImageInfo>(
        GLCommandCode::RESOLVE_IMAGE, offsetof(ResolveImageInfo, pRegions) + sizeof(ImageResolve) * info.regionCount);
    memcpy(p, &info, offsetof(ResolveImageInfo, pRegions));
    memcpy(&p->pRegions, info.pRegions, sizeof(ImageResolve) * info.regionCount);
}
void GLCommandBuffer::GenerateMipmap(const GenerateMipmapInfo &info) {
    auto p = AllocateCommand<GenerateMipmapInfo>(GLCommandCode::GENERATE_MIPMAP);
    *p = info;
}
}  // namespace Shit
