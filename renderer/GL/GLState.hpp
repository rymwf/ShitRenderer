/**
 * @file GLState.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <GL/glew.h>

#include <array>
#include <span>
#include <stack>
#include <unordered_map>
#include <vector>

#include "GLPrerequisites.hpp"

namespace Shit {
class GLStateManager {
    struct GLDrawBufferState {
        GLuint buffer{};
        std::stack<GLuint> drawBufferStack;
    } mDrawFramebufferState{};

    struct GLReadBufferState {
        GLuint buffer;
        std::stack<GLuint> readBufferStack;
    } mReadFramebufferState{};

    struct GLTextureState {
        struct StackEntry {
            GLuint unit;  //!< index of textureUnits	array, previous active unit
            GLenum target;
            GLuint texture{};
        };
        GLuint activeUnit;
        // target and texture
        std::array<std::array<GLuint, TEXTURE_TARGET_NUM>, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundTextures;
        std::stack<StackEntry> textureStack;
    } mTextureState{};

    struct GLSamplerState {
        struct StackEntry {
            GLuint unit;
            GLuint sampler{};
        };
        std::array<GLuint, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundSamplers{};
        std::stack<StackEntry> samplerStack{};
    } mSamplerState{};
    struct GLImageUnitState {
        struct StackEntry {
            GLuint unit;
            GLuint texture;
            GLint level;
            GLboolean layered;
            GLint layer;
            GLenum access;
            GLenum format;
        };
        std::array<StackEntry, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundTextures;  // TODO: same as texture??
        std::stack<StackEntry> imageStack;
    } mImageState{};
    struct GLRenderbufferState {
        GLuint curRenderbuffer{};
        std::stack<GLuint> renderbufferStatck;
    } mRenderbufferState{};
    struct GLBufferState {
        struct StackEntry {
            GLenum target;
            GLuint buffer{};
        };
        std::array<GLuint, BUFFER_TARGET_NUM> boundBuffers;
        std::stack<StackEntry> bufferStack;
    } mBufferState{};

    // std::

    struct GLCapbilityState {
        std::vector<bool> capbilityStates;
    } mCapabilityState{};

    struct GLPolygonState {
        GLenum mode;
        GLfloat offsetFactor;  // slopfactor
        GLfloat offsetUnits;   // constantfactor
        GLfloat clamp;
    } mPolygonState{};

    struct GLCullModeState {
        GLenum mode;
    } mCullModeState{};
    struct GLFrontFaceState {
        GLenum dir;
    } mFrontFaceState{};

    struct ViewportState {
        struct Viewport {
            GLfloat x;
            GLfloat y;
            GLfloat width;
            GLfloat height;
            GLfloat minDepth;
            GLfloat maxDepth;
        };
        std::array<Viewport, MAX_VIEWPORTS> viewports{};
    } mViewportState{};

    struct ScissorState {
        struct Scissor {
            // bool enabled;
            GLint x;
            GLint y;
            GLint width;
            GLint height;
        };
        std::array<Scissor, MAX_VIEWPORTS> scissors{};
    } mScissorState{};

    GLfloat mLineWidth;

    struct MultisampleState {
        GLfloat minSampleShading;
    } mMultisampleState{};

    struct StencilOpState {
        struct Op {
            GLenum failOp;       // stencil fail
            GLenum passOp;       // both pass
            GLenum depthFailOp;  // stencil pass depth fail
            GLenum compareOp;
            GLuint compareMask;
            GLint reference;
        };
        Op frontOpState;
        Op backOpState;
    } mStencilOpState{};

    struct DepthTestState {
        bool writeEnable;
        GLenum compareFunc;
    } mDepthTestState{};

    struct BlendState {
        struct AttachmentBlendState {
            bool enable;
            GLenum modeRGB;
            GLenum modeAlpha;
            GLenum srcRGB;
            GLenum srcAlpha;
            GLenum dstRGB;
            GLenum dstAlpha;
        };
        std::array<AttachmentBlendState, MAX_DRAW_BUFFERS> attachmentBlendStates;
        GLenum logicOp;
        float blendColor[4];
    } mBlendState{};

    struct AssemblyState {
        bool primitiveRestartEnable;
        GLenum primitiveTopology;
    } mAssemblyState{};

    struct ClipState {
        GLenum origin;
        GLenum depth;
    } mClipState{};

    struct IndexedTargetBindingState {
        struct Entry {
            GLuint buffer{};
            GLintptr offset;
            GLsizeiptr size;
        };
        std::array<Entry, MAX_UNIFORM_BLOCKS> uniformBufferStates;
        std::array<Entry, MAX_COMBINED_ATOMIC_COUNTERS> atomicCounterBufferStates;
        std::array<Entry, MAX_TRANSFORM_FEEDBACK_BUFFERS> transformFeedbackBufferStates;
        std::array<Entry, MAX_COMBINED_SHADER_STORAGE_BLOCKS> shaderStorageStates;
    } mIndexedTargetBindingState{};

    struct PatchState {
        GLint inputPatchVertexNum{};
        std::array<GLfloat, 4> outerLevels{};
        std::array<GLfloat, 2> innerLevels{};
    } mPatchState{};

    struct PipelineState {
        GLPipeline const *pipeline;
    } mPipelineState{};

public:
    GLStateManager();

    static void CheckError();

    // framebuffer
    void BindDrawFramebuffer(GLuint framebuffer);
    void PushDrawFramebuffer(GLuint framebuffer);
    void PopDrawFramebuffer();
    void BindReadFramebuffer(GLuint framebuffer);
    void PushReadFramebuffer(GLuint framebuffer);
    void PopReadFramebuffer();
    void NotifyReleasedFramebuffer(GLuint framebuffer);

    // renderbuffer
    void BindRenderbuffer(GLuint renderbuffer);
    void PushRenderbuffer(GLuint renderbuffer);
    void PopRenderbuffer();
    void NotifyReleasedRenderbuffer(GLuint renderbuffer);

    // buffer
    void BindBuffer(GLenum target, GLuint buffer);
    void PushBuffer(GLenum target, GLuint buffer);
    void PopBuffer();
    void NotifyReleaseBuffer(GLuint buffer);

    // pipeline
    void BindPipeline(GLPipeline const *pipeline);
    void NotifyReleasedPipeline(GLPipeline const *pipeline);
    constexpr GLPipeline const *GetPipeline() const { return mPipelineState.pipeline; }

    // vertexBuffers
    void BindVertexBuffers(BindVertexBuffersInfo const &info);
    void BindIndexBuffer(BindIndexBufferInfo const &info);

    GLenum GetIndexType() const;
    // constexpr uint64_t GetIndexOffset()
    //{
    //	return mPipelineState.indexBuffer.offset;
    // }

    // texture
    void BindTextureUnit(GLuint unit, GLenum target, GLuint texture);
    void PushTextureUnit(GLuint unit, GLenum target, GLuint texture);
    void PopTextureUnit();
    void NotifyReleasedTexture(GLenum target, GLuint texture);

    // image
    void BindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access,
                          GLenum format);
    void PushImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access,
                          GLenum format);
    void PopImageTexture();

    // sampler
    void BindSampler(GLuint unit, GLuint sampler);
    void PushSampler(GLuint unit, GLuint sampler);
    void PopSampler();
    void NotifyReleasedSampler(GLuint sampler);

    // capbility state
    void EnableCapability(GLenum cap);
    void DisableCapability(GLenum cap);
    void EnableCapabilityi(GLenum cap, GLuint index);
    void DisableCapabilityi(GLenum cap, GLuint index);
    void UpdateCapbilityState();

    // polygon mode
    void PolygonMode(GLenum mode);
    void PolygonOffSetClamp(GLfloat factor, GLfloat units, GLfloat clamp);

    // cull face mode
    void CullFace(GLenum mode);

    // front face
    void FrontFace(GLenum dir);

    /**
     * @brief Set the Viewports object
     *
     * @param first
     * @param count
     * @param v x,y,w,h,minDepth,maxDepth
     */
    void SetViewports(GLuint first, GLuint count, const float *v);

    /**
     * @brief Set the Scissors object
     *
     * @param first
     * @param count
     * @param v x,y, width,height
     */
    void SetScissors(GLuint first, GLuint count, const int32_t *v);

    void LineWidth(GLfloat width);

    void StencilOpState(GLenum face, GLenum func, GLint ref, GLuint mask, GLenum sfail, GLenum dpfail, GLenum dppass);
    void StencilCompareMask(GLenum face, GLuint mask);
    void StencilReference(GLenum face, GLint ref);

    void DepthFunc(GLenum func);

    void MinSampleShading(GLfloat value);

    // void BlendColor(const std::array<float, 4> &color);
    void BlendColor(const float color[4]);

    void EnableBlend(GLuint index);
    void DisableBlend(GLuint index);
    void BlendEquation(GLuint index, GLenum modeRGB, GLenum modeAlpha);
    void BlendFunc(GLuint index, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
    void LogicOp(GLenum op);

    void EnablePrimitiveRestart();
    void DisablePrimitiveRestart();
    void PrimitiveTopology(GLenum topology);
    constexpr GLenum GetPrimitiveTopology() { return mAssemblyState.primitiveTopology; }

    // clip
    void ClipControl(GLenum origin, GLenum depth);

    void BindBufferRange(GLenum target, GLuint binding, GLuint buffer, GLintptr offset, GLintptr size);

    void PatchInputVertexNum(GLint num);
    void PatchInnerLevels(const std::array<GLfloat, 2> &levels);
    void PatchOuterLevels(const std::array<GLfloat, 4> &levels);
};
}  // namespace Shit
