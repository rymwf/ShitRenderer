/**
 * @file GLCommandBuffer.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitCommandBuffer.hpp>

#include "GLPrerequisites.hpp"
namespace Shit {
class GLCommandPool;
class GLCommandBuffer final : public CommandBuffer {
public:
    enum class GLCommandCode : uint32_t {
        BEGIN_QUERY,
        BEGIN_RENDERPASS,
        BEGIN_TRANSFORMFEEDBACK,
        BIND_DESCRIPTORSETS,
        BIND_INDEXBUFFER,
        BIND_PIPELINE,
        BIND_TRANSFORMFEEDBACK_BUFFERS,
        BIND_VERTEX_BUFFER,
        BLIT_IMAGE,
        CLEAR_ATTACHMENTS,
        CLEAR_COLOR_IMAGE,
        CLEAR_DEPTH_STENCIL_IMAGE,
        COPY_BUFFER,
        COPY_BUFFER_TO_IMAGE,
        COPY_IMAGE,
        COPY_IMAGE_TO_BUFFER,
        COPY_QUERYPOOL_RESULTS,
        DISPATCH,
        DISPATCH_INDIRECT,
        DRAW,
        DRAW_INDIRECT,
        DRAW_INDIRECT_COUNT,
        DRAW_INDEXED,
        DRAW_INDEXED_INDIRECT,
        DRAW_INDEXED_INDIRECT_COUNT,
        END_QUERY,
        END_RENDERPASS,
        END_TRANSFORMFEEDBACK,
        EXECUTE_COMMANDS,
        FILL_BUFFER,
        GENERATE_MIPMAP,
        NEXT_SUBPASS,
        PIPELINE_BARRIER,
        PUSH_CONSTANTS,
        RESET_EVENT,
        RESET_QUERYPOOL,
        RESOLVE_IMAGE,
        SET_BLEND_CONSTANTS,
        SET_DEPTH_BIAS,
        SET_DEPTH_BOUNDS,
        SET_DEVICE_MASK,
        SET_EVENT,
        SET_LINE_WIDTH,
        SET_SCISSOR,
        SET_STENCIL_COMPARE_MASK,
        SET_STENCIL_REFERENCE,
        SET_STENCIL_WRITE_MASK,
        SET_VIEWPORT,
        UPDATE_BUFFER,
        WAIT_EVENT,
        WRITE_TIMESTAMP,
        Num,
    };

    using CommandFunctionPtr = size_t (GLCommandBuffer::*)(const void *);
    static CommandFunctionPtr mCommandFunctions[static_cast<size_t>(GLCommandCode::Num)];

private:
    std::vector<uint8_t> mBuffer;
    GLStateManager *mpStateManager;

    RenderPass const *mCurRenderPass{};
    uint32_t mCurSubpass{};
    IndexType mCurIndexType{};
    uint64_t mCurIndexOffset{};
    GLFramebuffer *mpCurFramebuffer{};

    class DummyFramebuffer {
    public:
        DummyFramebuffer();
        ~DummyFramebuffer();
        GLuint BindImage(GLStateManager *pManager, GLImage const *pImage, ImageAspectFlagBits imageAspect,
                         uint32_t levelCount, uint32_t const *levels);
        constexpr GLuint GetHandle() const { return _handle; }

    private:
        std::array<GLuint, 16> _colorAttachmentLevels{};
        GLImage const *_colorAttachment{};

        GLenum _depthStencilBindPoint{};
        GLImage const *_depthStencilAttachment{};
        GLuint _depthStencilLevel{~0u};

        GLuint _handle;

        void ImageDestroyListener(GLImage const *pImage);
    };
    DummyFramebuffer mDummyFramebuffer;
    DummyFramebuffer mDummyFramebuffer2;

    // TODO:: record framebuffer state
    // GLuint mDummyFramebuffer;
    // GLuint mDummyFramebuffer2;
    // std::unqiue_ptr<GLFramebuffer> mDummyFramebuffer;

    template <class T>
    T *AllocateCommand(GLCommandCode commandCode, size_t realSize = 0);

    //======
    size_t _BeginRenderPass(const void *params);
    size_t _BeginTransformFeedback(const void *params);
    size_t _BindIndexBuffer(const void *params);
    size_t _BindDescriptorSets(const void *params);
    size_t _BindPipeline(const void *params);
    size_t _BindTransformFeedBackBuffers(const void *params);
    size_t _BindVertexBuffers(const void *params);
    size_t _BlitImage(const void *params);
    size_t _ClearColorImage(const void *params);
    size_t _ClearDepthStencilImage(const void *params);
    size_t _ClearAttachments(const void *params);
    size_t _CopyBuffer(const void *params);
    size_t _CopyBufferToImage(const void *params);
    size_t _CopyImage(const void *params);
    size_t _CopyImageToBuffer(const void *params);
    size_t _Dispatch(const void *params);
    size_t _DispatchIndirect(const void *params);
    size_t _Draw(const void *params);
    size_t _DrawIndirect(const void *params);
    size_t _DrawIndirectCount(const void *params);
    size_t _DrawIndexed(const void *params);
    size_t _DrawIndexedIndirect(const void *params);
    size_t _DrawIndexedIndirectCount(const void *params);
    size_t _EndRenderPass(const void *params);
    size_t _EndTransformFeedback(const void *params);
    size_t _ExecuteCommands(const void *params);
    size_t _FillBuffer(const void *params);
    size_t _GenerateMipmap(const void *params);
    size_t _NextSubpass(const void *params);
    size_t _PipelineBarrier(const void *params);
    size_t _PushConstants(const void *params);
    size_t _ResolveImage(const void *params);
    size_t _SetBlendConstants(const void *params);
    size_t _SetDepthBias(const void *params);
    size_t _SetDepthBounds(const void *params);
    size_t _SetLineWidth(const void *params);
    size_t _SetStencilCompareMask(const void *params);
    size_t _SetStencilReference(const void *params);
    size_t _SetStencilWriteMask(const void *params);
    size_t _SetScissor(const void *params);
    size_t _SetViewport(const void *params);
    size_t _UpdateBuffer(const void *params);

    GLuint BindImageToFBO(GLuint fbo, Image const *pImage, uint32_t levelCount, uint32_t *levels);

public:
    GLCommandBuffer(GLStateManager *pStateManager, GLCommandPool *pCommandPool, CommandBufferLevel level);
    ~GLCommandBuffer();

    void Execute();

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
    void ResolveImage(const ResolveImageInfo &info);

    // others
    void GenerateMipmap(const GenerateMipmapInfo &info) override;
};
}  // namespace Shit
