/**
 * @file ShitCommandBuffer.hpp
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
class CommandBuffer {
protected:
    CommandPool *mCommandPool;
    CommandBufferLevel mLevel;

    CommandBuffer(CommandPool *commandPool, const CommandBufferLevel &level)
        : mCommandPool(commandPool), mLevel(level) {}

public:
    virtual ~CommandBuffer() {}

    constexpr CommandPool *GetCommandPool() const { return mCommandPool; }
    constexpr CommandBufferLevel GetLevel() const { return mLevel; }

    // not commands
    virtual void Reset(CommandBufferResetFlatBits flags = {}) = 0;
    virtual void Begin(const CommandBufferBeginInfo &beginInfo = {}) = 0;
    virtual void End() = 0;

    // commandbuffer
    // TODO: setdevice mask
    virtual void ExecuteCommands(const ExecuteCommandsInfo &info) = 0;

    // renderpass
    virtual void BeginRenderPass(const BeginRenderPassInfo &beginInfo) = 0;
    virtual void EndRenderPass() = 0;
    virtual void NextSubpass(SubpassContents subpassContents) = 0;

    // pipelines
    virtual void BindPipeline(const BindPipelineInfo &info) = 0;

    // clear commands
    virtual void ClearColorImage(const ClearColorImageInfo &info) = 0;
    virtual void ClearDepthStencilImage(const ClearDepthStencilImageInfo &info) = 0;
    virtual void ClearAttachments(const ClearAttachmentsInfo &info) = 0;
    virtual void FillBuffer(const FillBufferInfo &) = 0;
    virtual void UpdateBuffer(const UpdateBufferInfo &info) = 0;

    // copy commands
    virtual void CopyBuffer(const CopyBufferInfo &copyInfo) = 0;
    virtual void CopyImage(const CopyImageInfo &copyInfo) = 0;
    virtual void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) = 0;
    virtual void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) = 0;
    virtual void BlitImage(const BlitImageInfo &blitInfo) = 0;

    // vertex input description
    virtual void BindVertexBuffers(const BindVertexBuffersInfo &info) = 0;

    // resource descriptors
    virtual void BindDescriptorSets(const BindDescriptorSetsInfo &info) = 0;
    virtual void PushConstants(const PushConstantInfo &info) = 0;
    virtual void PushDescriptorSet(const PushDescriptorSetInfo &info) = 0;

    // draw commands
    virtual void BindIndexBuffer(const BindIndexBufferInfo &info) = 0;

    virtual void Draw(const DrawIndirectCommand &info) = 0;
    // same as drawIndexed
    virtual void Draw(const DrawIndexedIndirectCommand &info) = 0;
    virtual void DrawIndexed(const DrawIndexedIndirectCommand &info) = 0;

    virtual void DrawIndirect(const DrawIndirectInfo &info) = 0;
    // same as drawIndirectCount
    virtual void DrawIndirect(const DrawIndirectCountInfo &info) = 0;
    virtual void DrawIndirectCount(const DrawIndirectCountInfo &info) = 0;

    virtual void DrawIndexedIndirect(const DrawIndirectInfo &info) = 0;
    // same as drawIndexedIndirectCount
    virtual void DrawIndexedIndirect(const DrawIndirectCountInfo &info) = 0;
    virtual void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) = 0;

    // synchronization and cache control
    // TODO: set reset wait event
    virtual void PipelineBarrier(const PipelineBarrierInfo &info) = 0;

    // compute shader dispatch
    virtual void Dispatch(const DispatchInfo &info) = 0;
    virtual void DispatchIndirect(const DispatchIndirectInfo &info) = 0;

    // transform feedback
    virtual void BindTransformFeedbackBuffers(const BindTransformFeedbackBuffersInfo &info) = 0;
    virtual void BeginTransformFeedback(const BeginTransformFeedbackInfo &info) = 0;
    virtual void EndTransformFeedback(const EndTransformFeedbackInfo &info) = 0;

    // fixed function vertex postprocessing
    virtual void SetViewport(const SetViewPortInfo &info) = 0;

    // rasterization
    virtual void SetLineWidth(float lineWidth) = 0;
    virtual void SetDepthBias(const SetDepthBiasInfo &info) = 0;

    // blend factors
    virtual void SetBlendConstants(const float blendConstants[4]) = 0;

    // fragment operations
    virtual void SetScissor(const SetScissorInfo &info) = 0;
    virtual void SetDepthBounds(const SetDepthBoundsInfo &info) = 0;
    virtual void SetStencilCompareMask(const SetStencilCompareMaskInfo &info) = 0;
    virtual void SetStencilWriteMask(const SetStencilWriteMaskInfo &info) = 0;
    virtual void SetStencilReference(const SetStencilReferenceInfo &info) = 0;

    // resolve image
    virtual void ResolveImage(const ResolveImageInfo &info) = 0;

    // other
    virtual void GenerateMipmap(const GenerateMipmapInfo &info) = 0;
};
}  // namespace Shit