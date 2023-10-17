/**
 * @file VKCommandBuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitCommandBuffer.hpp>

#include "VKDeviceObject.hpp"
#include "VKPrerequisites.hpp"
namespace Shit {
class VKCommandPool;
class VKCommandBuffer final : public CommandBuffer, public VKDeviceObject {
    VkCommandBuffer mHandle;

public:
    VKCommandBuffer(VKDevice *pDevice, VKCommandPool *pCommandPool, CommandBufferLevel level);
    constexpr VkCommandBuffer GetHandle() const { return mHandle; }
    ~VKCommandBuffer() override {}
    void Destroy();

    // not commands
    void Reset(CommandBufferResetFlatBits flags) override;
    void Begin(const CommandBufferBeginInfo &beginInfo) override;
    void End() override;

    void ExecuteCommands(const ExecuteCommandsInfo &info) override;

    // renderpass
    void BeginRenderPass(const BeginRenderPassInfo &beginInfo) override;
    void EndRenderPass() override;
    void NextSubpass(SubpassContents subpassContents) override;

    // pipelines
    void BindPipeline(const BindPipelineInfo &info) override;

    // clear commands
    void ClearColorImage(const ClearColorImageInfo &info) override;
    void ClearDepthStencilImage(const ClearDepthStencilImageInfo &info) override;
    void ClearAttachments(const ClearAttachmentsInfo &info) override;
    void FillBuffer(const FillBufferInfo &) override;
    void UpdateBuffer(const UpdateBufferInfo &info) override;

    // copy commands
    void CopyBuffer(const CopyBufferInfo &copyInfo) override;
    void CopyImage(const CopyImageInfo &copyInfo) override;
    void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
    void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
    void BlitImage(const BlitImageInfo &blitInfo) override;

    // vertex input description
    void BindVertexBuffers(const BindVertexBuffersInfo &info) override;

    // resource descriptors
    void BindDescriptorSets(const BindDescriptorSetsInfo &info) override;
    void PushConstants(const PushConstantInfo &info) override;
    void PushDescriptorSet(const PushDescriptorSetInfo &info) override;

    // draw commands
    void BindIndexBuffer(const BindIndexBufferInfo &info) override;

    void Draw(const DrawIndirectCommand &info) override;
    void Draw(const DrawIndexedIndirectCommand &info) override;  // same as drawIndexed
    void DrawIndexed(const DrawIndexedIndirectCommand &info) override;

    void DrawIndirect(const DrawIndirectInfo &info) override;
    void DrawIndirect(const DrawIndirectCountInfo &info) override;  // same as drawIndirectCount
    void DrawIndirectCount(const DrawIndirectCountInfo &info) override;

    void DrawIndexedIndirect(const DrawIndirectInfo &info) override;
    void DrawIndexedIndirect(const DrawIndirectCountInfo &info) override;  // same as drawIndexedIndirectCount
    void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) override;

    // synchronization and cache control
    void PipelineBarrier(const PipelineBarrierInfo &info) override;

    // compute shader dispatch
    void Dispatch(const DispatchInfo &info) override;
    void DispatchIndirect(const DispatchIndirectInfo &info) override;

    // transform feedback
    void BindTransformFeedbackBuffers(const BindTransformFeedbackBuffersInfo &info) override;
    void BeginTransformFeedback(const BeginTransformFeedbackInfo &info) override;
    void EndTransformFeedback(const EndTransformFeedbackInfo &info) override;

    // fixed function vertex postprocessing
    void SetViewport(const SetViewPortInfo &info) override;

    // rasterization
    void SetLineWidth(float lineWidth) override;
    void SetDepthBias(const SetDepthBiasInfo &info) override;

    // blend factors
    void SetBlendConstants(const float blendConstants[4]) override;

    // fragment operations
    void SetScissor(const SetScissorInfo &info) override;
    void SetDepthBounds(const SetDepthBoundsInfo &info) override;
    void SetStencilCompareMask(const SetStencilCompareMaskInfo &info) override;
    void SetStencilWriteMask(const SetStencilWriteMaskInfo &info) override;
    void SetStencilReference(const SetStencilReferenceInfo &info) override;

    // resolve image
    void ResolveImage(const ResolveImageInfo &info) override;

    void GenerateMipmap(const GenerateMipmapInfo &info) override;
};
}  // namespace Shit
