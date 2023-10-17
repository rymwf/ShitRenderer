/**
 * @file GLState.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLState.hpp"

#include "GLPipeline.hpp"

namespace Shit {
static constexpr GLenum glBufferIndexedTargetArray[]{GL_ATOMIC_COUNTER_BUFFER, GL_SHADER_STORAGE_BUFFER,
                                                     GL_TRANSFORM_FEEDBACK_BUFFER, GL_UNIFORM_BUFFER};
static const std::unordered_map<GLenum, int> glTextureTargetTable{
    {GL_TEXTURE_1D, 0},
    {GL_TEXTURE_2D, 1},
    {GL_TEXTURE_3D, 2},
    {GL_TEXTURE_CUBE_MAP, 3},
    {GL_TEXTURE_1D_ARRAY, 4},
    {GL_TEXTURE_2D_ARRAY, 5},
    {GL_TEXTURE_CUBE_MAP_ARRAY, 6},
    {GL_TEXTURE_2D_MULTISAMPLE, 7},
    {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 8},
    {GL_TEXTURE_BUFFER, 9},
    {GL_TEXTURE_RECTANGLE, 10},
};
static const std::unordered_map<GLenum, int> glCapabilityTable{
    //{GL_CLIP_DISTANCE,},
    {GL_BLEND, 0},
    {GL_COLOR_LOGIC_OP, 1},
    {GL_CULL_FACE, 2},
    {GL_DEBUG_OUTPUT, 3},
    {GL_DEBUG_OUTPUT_SYNCHRONOUS, 4},
    {GL_DEPTH_CLAMP, 5},
    {GL_DEPTH_TEST, 6},
    {GL_DITHER, 7},
    {GL_FRAMEBUFFER_SRGB, 8},
    {GL_LINE_SMOOTH, 9},
    {GL_MULTISAMPLE, 10},
    {GL_POLYGON_OFFSET_FILL, 11},
    {GL_POLYGON_OFFSET_LINE, 12},
    {GL_POLYGON_OFFSET_POINT, 13},
    {GL_POLYGON_SMOOTH, 14},
    {GL_PRIMITIVE_RESTART, 15},
    {GL_PRIMITIVE_RESTART_FIXED_INDEX, 16},
    {GL_RASTERIZER_DISCARD, 17},
    {GL_SAMPLE_ALPHA_TO_COVERAGE, 18},
    {GL_SAMPLE_ALPHA_TO_ONE, 19},
    {GL_SAMPLE_COVERAGE, 20},
    {GL_SAMPLE_SHADING, 21},
    {GL_SAMPLE_MASK, 22},
    {GL_SCISSOR_TEST, 23},
    {GL_STENCIL_TEST, 24},
    {GL_TEXTURE_CUBE_MAP_SEAMLESS, 25},
    {GL_PROGRAM_POINT_SIZE, 26},
};
static const std::unordered_map<GLenum, int> glBufferBindTargetMap{
    {GL_ARRAY_BUFFER, 0},
    {GL_ATOMIC_COUNTER_BUFFER, 1},
    {GL_COPY_READ_BUFFER, 2},
    {GL_COPY_WRITE_BUFFER, 3},
    {GL_DISPATCH_INDIRECT_BUFFER, 4},
    {GL_DRAW_INDIRECT_BUFFER, 5},
    {GL_ELEMENT_ARRAY_BUFFER, 6},
    {GL_PARAMETER_BUFFER, 7},
    {GL_PIXEL_PACK_BUFFER, 8},
    {GL_PIXEL_UNPACK_BUFFER, 9},
    {GL_QUERY_BUFFER, 10},
    {GL_SHADER_STORAGE_BUFFER, 11},
    {GL_TEXTURE_BUFFER, 12},
    {GL_TRANSFORM_FEEDBACK_BUFFER, 13},
    {GL_UNIFORM_BUFFER, 14},
};
void GLStateManager::CheckError() {
    auto ret = glGetError();
    switch (ret) {
        case GL_CONTEXT_LOST:
            ST_LOG("GL_CONTEXT_LOST")
            break;
        case GL_INVALID_ENUM:
            ST_LOG("GL_INVALID_ENUM")
            break;
        case GL_INVALID_VALUE:
            ST_LOG("GL_INVALID_VALUE")
            break;
        case GL_INVALID_OPERATION:
            ST_LOG("GL_INVALID_OPERATION")
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            ST_LOG("GL_INVALID_FRAMEBUFFER_OPERATION")
            break;
        case GL_OUT_OF_MEMORY:
            ST_LOG("GL_OUT_OF_MEMORY")
            break;
        case GL_STACK_OVERFLOW:
            ST_LOG("GL_STACK_OVERFLOW")
            break;
        case GL_STACK_UNDERFLOW:
            ST_LOG("GL_STACK_UNDERFLOW")
            break;
        case GL_NO_ERROR:
            return;
    }
}
GLStateManager::GLStateManager() { UpdateCapbilityState(); }
void GLStateManager::BindDrawFramebuffer(GLuint framebuffer) {
    if (mDrawFramebufferState.buffer != framebuffer) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        mDrawFramebufferState.buffer = framebuffer;
    }
}
void GLStateManager::PushDrawFramebuffer(GLuint framebuffer) {
    mDrawFramebufferState.drawBufferStack.emplace(mDrawFramebufferState.buffer);
    BindDrawFramebuffer(framebuffer);
}
void GLStateManager::PopDrawFramebuffer() {
    BindDrawFramebuffer(mDrawFramebufferState.drawBufferStack.top());
    mDrawFramebufferState.drawBufferStack.pop();
}

void GLStateManager::BindReadFramebuffer(GLuint framebuffer) {
    if (mReadFramebufferState.buffer != framebuffer) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        mReadFramebufferState.buffer = framebuffer;
    }
}
void GLStateManager::PushReadFramebuffer(GLuint framebuffer) {
    mReadFramebufferState.readBufferStack.emplace(mReadFramebufferState.buffer);
    BindReadFramebuffer(framebuffer);
}
void GLStateManager::PopReadFramebuffer() {
    BindReadFramebuffer(mReadFramebufferState.readBufferStack.top());
    mReadFramebufferState.readBufferStack.pop();
}

void GLStateManager::NotifyReleasedFramebuffer(GLuint framebuffer) {
    if (mDrawFramebufferState.buffer == framebuffer) mDrawFramebufferState.buffer = 0;
    if (mReadFramebufferState.buffer == framebuffer) mReadFramebufferState.buffer = 0;
}
void GLStateManager::BindRenderbuffer(GLuint renderbuffer) {
    if (mRenderbufferState.curRenderbuffer != renderbuffer) {
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        mRenderbufferState.curRenderbuffer = renderbuffer;
    }
}
void GLStateManager::PushRenderbuffer(GLuint renderbuffer) {
    mRenderbufferState.renderbufferStatck.emplace(mRenderbufferState.curRenderbuffer);
    BindRenderbuffer(renderbuffer);
}
void GLStateManager::PopRenderbuffer() {
    BindRenderbuffer(mRenderbufferState.renderbufferStatck.top());
    mRenderbufferState.renderbufferStatck.pop();
}
void GLStateManager::NotifyReleasedRenderbuffer(GLuint renderbuffer) {
    if (mRenderbufferState.curRenderbuffer == renderbuffer) mRenderbufferState.curRenderbuffer = 0;
}

void GLStateManager::BindBuffer(GLenum target, GLuint buffer) {
    auto index = glBufferBindTargetMap.at(target);
    if (mBufferState.boundBuffers[index] != buffer) {
        glBindBuffer(target, buffer);
        mBufferState.boundBuffers[index] = buffer;
    }
}
void GLStateManager::PushBuffer(GLenum target, GLuint buffer) {
    auto index = glBufferBindTargetMap.at(target);
    mBufferState.bufferStack.emplace(GLBufferState::StackEntry{target, mBufferState.boundBuffers[index]});
    if (mBufferState.boundBuffers[index] != buffer) {
        glBindBuffer(target, buffer);
        mBufferState.boundBuffers[index] = buffer;
    }
}
void GLStateManager::PopBuffer() {
    auto &&a = mBufferState.bufferStack.top();
    BindBuffer(a.target, a.buffer);
    mBufferState.bufferStack.pop();
}
void GLStateManager::NotifyReleaseBuffer(GLuint buffer) {
    for (auto &&e : mBufferState.boundBuffers) {
        if (e == buffer) e = 0;
    }
    for (auto &&e : mIndexedTargetBindingState.uniformBufferStates) {
        if (e.buffer == buffer) e = {};
    }
    for (auto &&e : mIndexedTargetBindingState.atomicCounterBufferStates) {
        if (e.buffer == buffer) e = {};
    }
    for (auto &&e : mIndexedTargetBindingState.transformFeedbackBufferStates) {
        if (e.buffer == buffer) e = {};
    }
    for (auto &&e : mIndexedTargetBindingState.shaderStorageStates) {
        if (e.buffer == buffer) e = {};
    }
}
void GLStateManager::BindTextureUnit(GLuint unit, GLenum target, GLuint texture) {
    auto &&a = mTextureState.boundTextures[unit];
    auto index = glTextureTargetTable.at(target);
    if (a[index] != texture) {
        if (mTextureState.activeUnit != unit) {
            glActiveTexture(GL_TEXTURE0 + unit);
            mTextureState.activeUnit = unit;
        }
        glBindTexture(target, texture);
        a[index] = texture;
    }
}
void GLStateManager::PushTextureUnit(GLuint unit, GLenum target, GLuint texture) {
    auto &&a = mTextureState.boundTextures[unit];
    auto index = glTextureTargetTable.at(target);
    mTextureState.textureStack.emplace(GLTextureState::StackEntry{unit, target, a[index]});
    BindTextureUnit(unit, target, texture);
}
void GLStateManager::PopTextureUnit() {
    auto &&a = mTextureState.textureStack.top();
    BindTextureUnit(a.unit, a.target, a.texture);
    mTextureState.textureStack.pop();
}
void GLStateManager::NotifyReleasedTexture(GLenum target, GLuint texture) {
    auto index = glTextureTargetTable.at(target);
    for (auto &&e : mTextureState.boundTextures) {
        if (e[index] == texture) {
            e[index] = 0;
            break;
        }
    }
    for (auto &&e : mImageState.boundTextures) {
        if (e.texture == texture) {
            e.texture = 0;
            break;
        }
    }
}
void GLStateManager::BindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
                                      GLenum access, GLenum format) {
    auto &&a = mImageState.boundTextures[unit];
    if (a.texture != texture || a.level != level || a.layered != layered || a.layer != layer || a.access != access ||
        a.format != format) {
        glBindImageTexture(unit, texture, level, layered, layer, access, format);
        a = {unit, texture, level, layered, layer, access, format};
    }
}
void GLStateManager::PushImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer,
                                      GLenum access, GLenum format) {
    mImageState.imageStack.emplace(mImageState.boundTextures[unit]);
    BindImageTexture(unit, texture, level, layered, layer, access, format);
}
void GLStateManager::PopImageTexture() {
    auto &&a = mImageState.imageStack.top();
    BindImageTexture(a.unit, a.texture, a.level, a.layered, a.layer, a.access, a.format);
    mImageState.imageStack.pop();
}
void GLStateManager::BindSampler(GLuint unit, GLuint sampler) {
    if (mSamplerState.boundSamplers[unit] != sampler) {
        glBindSampler(unit, sampler);
        mSamplerState.boundSamplers[unit] = sampler;
    }
}
void GLStateManager::PushSampler(GLuint unit, GLuint sampler) {
    mSamplerState.samplerStack.emplace(GLSamplerState::StackEntry{unit, sampler});
    BindSampler(unit, sampler);
}
void GLStateManager::PopSampler() {
    auto &&a = mSamplerState.samplerStack.top();
    BindSampler(a.unit, a.sampler);
    mSamplerState.samplerStack.pop();
}
void GLStateManager::NotifyReleasedSampler(GLuint sampler) {
    for (auto &&e : mSamplerState.boundSamplers) {
        if (e == sampler) e = 0;
    }
}
void GLStateManager::BindPipeline(GLPipeline const *pipeline) {
    if (mPipelineState.pipeline != pipeline) {
        pipeline->Bind();
        mPipelineState.pipeline = pipeline;
    }
}
void GLStateManager::NotifyReleasedPipeline(GLPipeline const *pipeline) {
    if (mPipelineState.pipeline == pipeline) mPipelineState = {};
}
void GLStateManager::BindVertexBuffers(BindVertexBuffersInfo const &info) {
    const_cast<GLGraphicsPipeline *>(dynamic_cast<GLGraphicsPipeline const *>(GetPipeline()))->BindVertexBuffers(info);
}
GLenum GLStateManager::GetIndexType() const {
    return dynamic_cast<GLGraphicsPipeline const *>(GetPipeline())->GetIndexType();
}
void GLStateManager::BindIndexBuffer(BindIndexBufferInfo const &info) {
    const_cast<GLGraphicsPipeline *>(dynamic_cast<GLGraphicsPipeline const *>(GetPipeline()))->BindIndexBuffer(info);
}
void GLStateManager::EnableCapability(GLenum cap) {
    auto index = glCapabilityTable.at(cap);
    if (!mCapabilityState.capbilityStates[index]) {
        glEnable(cap);
        mCapabilityState.capbilityStates[index] = true;
    }
}
void GLStateManager::DisableCapability(GLenum cap) {
    auto index = glCapabilityTable.at(cap);
    if (mCapabilityState.capbilityStates[index]) {
        glDisable(cap);
        mCapabilityState.capbilityStates[index] = false;
    }
}
void GLStateManager::EnableCapabilityi(GLenum, GLuint) { ST_THROW("EnableCapabilityi not implemented yet") }
void GLStateManager::DisableCapabilityi(GLenum, GLuint) { ST_THROW("DisableCapabilityi not implemented yet") }
void GLStateManager::UpdateCapbilityState() {
    mCapabilityState.capbilityStates.resize(glCapabilityTable.size());
    for (auto &&e : glCapabilityTable) {
        mCapabilityState.capbilityStates[e.second] = glIsEnabled(e.first);
    }
}
void GLStateManager::PolygonMode(GLenum mode) {
    if (mPolygonState.mode != mode) {
        glPolygonMode(GL_FRONT_AND_BACK, mode);
        mPolygonState.mode = mode;
    }
}
void GLStateManager::PolygonOffSetClamp(GLfloat factor, GLfloat units, GLfloat clamp) {
    if (mPolygonState.offsetFactor != factor || mPolygonState.offsetUnits != units || mPolygonState.clamp != clamp) {
        glPolygonOffsetClamp(factor, units, clamp);
        mPolygonState.offsetFactor = factor;
        mPolygonState.offsetUnits = units;
        mPolygonState.clamp = clamp;
    }
}
// cull face mode
void GLStateManager::CullFace(GLenum mode) {
    if (mCullModeState.mode != mode) {
        glCullFace(mode);
        mCullModeState.mode = mode;
    }
}

// front face
void GLStateManager::FrontFace(GLenum dir) {
    if (mFrontFaceState.dir != dir) {
        glFrontFace(dir);
        mFrontFaceState.dir = dir;
    }
}
void GLStateManager::SetViewports(GLuint first, GLuint count, const float *v) {
    GLuint i = 0;
    while (i < count) {
        if (mViewportState.viewports[first + i].x != v[i * 6 + 0] ||
            mViewportState.viewports[first + i].y != v[i * 6 + 1] ||
            mViewportState.viewports[first + i].width != v[i * 6 + 2] ||
            mViewportState.viewports[first + i].height != v[i * 6 + 3]) {
            glViewportIndexedf(first + i, v[i * 6 + 0], v[i * 6 + 1], v[i * 6 + 2], v[i * 6 + 3]);
            mViewportState.viewports[first + i].x = v[i * 6 + 0];
            mViewportState.viewports[first + i].y = v[i * 6 + 1];
            mViewportState.viewports[first + i].width = v[i * 6 + 2];
            mViewportState.viewports[first + i].height = v[i * 6 + 3];
        }
        if (mViewportState.viewports[first + i].minDepth != v[i * 6 + 4] ||
            mViewportState.viewports[first + i].maxDepth != v[i * 6 + 5]) {
            glDepthRangeIndexed(first + i, v[i * 6 + 4], v[i * 6 + 5]);
            mViewportState.viewports[first + i].minDepth = v[i * 6 + 4];
            mViewportState.viewports[first + i].maxDepth = v[i * 6 + 5];
        }
        ++i;
    }
}
void GLStateManager::SetScissors(GLuint first, GLuint count, const int32_t *v) {
    GLuint i = 0;
    while (i < count) {
        if (mScissorState.scissors[first + i].x != v[i * 4 + 0] ||
            mScissorState.scissors[first + i].y != v[i * 4 + 1] ||
            mScissorState.scissors[first + i].width != v[i * 4 + 2] ||
            mScissorState.scissors[first + i].height != v[i * 4 + 3]) {
            glScissorIndexed(first + i, v[i * 4 + 0], v[i * 4 + 1], v[i * 4 + 2], v[i * 4 + 3]);
            mScissorState.scissors[first + i].x = v[i * 4 + 0];
            mScissorState.scissors[first + i].y = v[i * 4 + 1];
            mScissorState.scissors[first + i].width = v[i * 4 + 2];
            mScissorState.scissors[first + i].height = v[i * 4 + 3];
        }
        ++i;
    }
}
void GLStateManager::LineWidth(GLfloat width) {
    if (mLineWidth != width) {
        glLineWidth(width);
        mLineWidth = width;
    }
}
void GLStateManager::StencilOpState(GLenum face, GLenum func, GLint ref, GLuint mask, GLenum sfail, GLenum dpfail,
                                    GLenum dppass) {
    if (face == GL_FRONT) {
        if (mStencilOpState.frontOpState.compareOp != func || mStencilOpState.frontOpState.reference != ref ||
            mStencilOpState.frontOpState.compareMask != mask) {
            glStencilFuncSeparate(face, func, ref, mask);
            mStencilOpState.frontOpState.compareOp = func;
            mStencilOpState.frontOpState.reference = ref;
            mStencilOpState.frontOpState.compareMask = mask;
        }
        if (mStencilOpState.frontOpState.failOp != sfail || mStencilOpState.frontOpState.passOp != dpfail ||
            mStencilOpState.frontOpState.depthFailOp != dpfail) {
            glStencilOpSeparate(face, sfail, dpfail, dppass);
            mStencilOpState.frontOpState.failOp = sfail;
            mStencilOpState.frontOpState.passOp = dpfail;
            mStencilOpState.frontOpState.depthFailOp = dpfail;
        }
    } else if (face == GL_BACK) {
        if (mStencilOpState.backOpState.compareOp != func || mStencilOpState.backOpState.reference != ref ||
            mStencilOpState.backOpState.compareMask != mask) {
            glStencilFuncSeparate(face, func, ref, mask);
            mStencilOpState.backOpState.compareOp = func;
            mStencilOpState.backOpState.reference = ref;
            mStencilOpState.backOpState.compareMask = mask;
        }
        if (mStencilOpState.backOpState.failOp != sfail || mStencilOpState.backOpState.passOp != dpfail ||
            mStencilOpState.backOpState.depthFailOp != dpfail) {
            glStencilOpSeparate(face, sfail, dpfail, dppass);
            mStencilOpState.backOpState.failOp = sfail;
            mStencilOpState.backOpState.passOp = dpfail;
            mStencilOpState.backOpState.depthFailOp = dpfail;
        }
    } else if (face == GL_FRONT_AND_BACK) {
        StencilOpState(GL_FRONT, func, ref, mask, sfail, dpfail, dppass);
        StencilOpState(GL_BACK, func, ref, mask, sfail, dpfail, dppass);
    }
}
void GLStateManager::StencilCompareMask(GLenum face, GLuint mask) {
    if (face == GL_FRONT) {
        if (mStencilOpState.frontOpState.compareMask != mask) {
            glStencilFuncSeparate(face, mStencilOpState.frontOpState.compareOp, mStencilOpState.frontOpState.reference,
                                  mask);
            mStencilOpState.frontOpState.compareMask = mask;
        }
    } else if (face == GL_BACK) {
        if (mStencilOpState.backOpState.compareMask != mask) {
            glStencilFuncSeparate(face, mStencilOpState.backOpState.compareOp, mStencilOpState.backOpState.reference,
                                  mask);
            mStencilOpState.backOpState.compareMask = mask;
        }
    }
}
void GLStateManager::StencilReference(GLenum face, GLint ref) {
    if (face == GL_FRONT) {
        if (mStencilOpState.frontOpState.reference != ref) {
            glStencilFuncSeparate(face, mStencilOpState.frontOpState.compareOp, ref,
                                  mStencilOpState.frontOpState.compareMask);
            mStencilOpState.frontOpState.reference = ref;
        }
    } else if (face == GL_BACK) {
        if (mStencilOpState.backOpState.reference != ref) {
            glStencilFuncSeparate(face, mStencilOpState.backOpState.compareOp, ref,
                                  mStencilOpState.backOpState.compareMask);
            mStencilOpState.backOpState.reference = ref;
        }
    }
}
void GLStateManager::DepthFunc(GLenum func) {
    if (mDepthTestState.compareFunc != func) {
        glDepthFunc(func);
        mDepthTestState.compareFunc = func;
    }
}
void GLStateManager::MinSampleShading(GLfloat value) {
    if (mMultisampleState.minSampleShading != value) {
        glMinSampleShading(value);
        mMultisampleState.minSampleShading = value;
    }
}
void GLStateManager::BlendColor(const float color[4]) {
    if (mBlendState.blendColor[0] != color[0] && mBlendState.blendColor[1] != color[1] &&
        mBlendState.blendColor[2] != color[2] && mBlendState.blendColor[3] != color[3]) {
        glBlendColor(color[0], color[1], color[2], color[3]);
        memcpy(mBlendState.blendColor, color, sizeof(float) * 4);
    }
}
void GLStateManager::EnableBlend(GLuint index) {
    if (!mBlendState.attachmentBlendStates[index].enable) {
        glEnablei(GL_BLEND, index);
        mBlendState.attachmentBlendStates[index].enable = true;
    }
}
void GLStateManager::DisableBlend(GLuint index) {
    if (mBlendState.attachmentBlendStates[index].enable) {
        glDisablei(GL_BLEND, index);
        mBlendState.attachmentBlendStates[index].enable = false;
    }
}
void GLStateManager::BlendEquation(GLuint index, GLenum modeRGB, GLenum modeAlpha) {
    if (mBlendState.attachmentBlendStates[index].modeRGB != modeRGB ||
        mBlendState.attachmentBlendStates[index].modeAlpha != modeAlpha) {
        glBlendEquationSeparatei(index, modeRGB, modeAlpha);
        mBlendState.attachmentBlendStates[index].modeRGB = modeRGB;
        mBlendState.attachmentBlendStates[index].modeAlpha = modeAlpha;
    }
}
void GLStateManager::BlendFunc(GLuint index, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha) {
    if (mBlendState.attachmentBlendStates[index].srcRGB != srcRGB ||
        mBlendState.attachmentBlendStates[index].dstRGB != dstRGB ||
        mBlendState.attachmentBlendStates[index].srcAlpha != srcAlpha ||
        mBlendState.attachmentBlendStates[index].dstAlpha != dstAlpha) {
        glBlendFuncSeparatei(index, srcRGB, dstRGB, srcAlpha, dstAlpha);
        mBlendState.attachmentBlendStates[index].srcRGB = srcRGB;
        mBlendState.attachmentBlendStates[index].dstRGB = dstRGB;
        mBlendState.attachmentBlendStates[index].srcAlpha = srcAlpha;
        mBlendState.attachmentBlendStates[index].dstAlpha = dstAlpha;
    }
}
void GLStateManager::LogicOp(GLenum op) {
    if (mBlendState.logicOp != op) {
        glLogicOp(op);
        mBlendState.logicOp = op;
    }
}
void GLStateManager::EnablePrimitiveRestart() {
    if (!mAssemblyState.primitiveRestartEnable) {
        glEnable(GL_PRIMITIVE_RESTART);
        mAssemblyState.primitiveRestartEnable = true;
    }
}
void GLStateManager::DisablePrimitiveRestart() {
    if (mAssemblyState.primitiveRestartEnable) {
        glDisable(GL_PRIMITIVE_RESTART);
        mAssemblyState.primitiveRestartEnable = false;
    }
}
void GLStateManager::PrimitiveTopology(GLenum topology) { mAssemblyState.primitiveTopology = topology; }
void GLStateManager::ClipControl(GLenum origin, GLenum depth) {
    if (mClipState.origin != origin || mClipState.depth != depth) {
        glClipControl(origin, depth);
        mClipState.origin = origin;
        mClipState.depth = depth;
    }
}
void GLStateManager::BindBufferRange(GLenum target, GLuint binding, GLuint buffer, GLintptr offset, GLintptr size) {
    if (target == GL_UNIFORM_BUFFER) {
        if (mIndexedTargetBindingState.uniformBufferStates[binding].buffer != buffer ||
            mIndexedTargetBindingState.uniformBufferStates[binding].offset != offset ||
            mIndexedTargetBindingState.uniformBufferStates[binding].size != size) {
            glBindBufferRange(target, binding, buffer, offset, size);
            mIndexedTargetBindingState.uniformBufferStates[binding].buffer = buffer;
            mIndexedTargetBindingState.uniformBufferStates[binding].offset = offset;
            mIndexedTargetBindingState.uniformBufferStates[binding].size = size;
        }
    } else if (target == GL_ATOMIC_COUNTER_BUFFER) {
        if (mIndexedTargetBindingState.atomicCounterBufferStates[binding].buffer != buffer ||
            mIndexedTargetBindingState.atomicCounterBufferStates[binding].offset != offset ||
            mIndexedTargetBindingState.atomicCounterBufferStates[binding].size != size) {
            glBindBufferRange(target, binding, buffer, offset, size);
            mIndexedTargetBindingState.atomicCounterBufferStates[binding].buffer = buffer;
            mIndexedTargetBindingState.atomicCounterBufferStates[binding].offset = offset;
            mIndexedTargetBindingState.atomicCounterBufferStates[binding].size = size;
        }
    } else if (target == GL_TRANSFORM_FEEDBACK_BUFFER) {
        if (mIndexedTargetBindingState.transformFeedbackBufferStates[binding].buffer != buffer ||
            mIndexedTargetBindingState.transformFeedbackBufferStates[binding].offset != offset ||
            mIndexedTargetBindingState.transformFeedbackBufferStates[binding].size != size) {
            glBindBufferRange(target, binding, buffer, offset, size);
            mIndexedTargetBindingState.transformFeedbackBufferStates[binding].buffer = buffer;
            mIndexedTargetBindingState.transformFeedbackBufferStates[binding].offset = offset;
            mIndexedTargetBindingState.transformFeedbackBufferStates[binding].size = size;
        }
    } else if (target == GL_SHADER_STORAGE_BUFFER) {
        if (mIndexedTargetBindingState.shaderStorageStates[binding].buffer != buffer ||
            mIndexedTargetBindingState.shaderStorageStates[binding].offset != offset ||
            mIndexedTargetBindingState.shaderStorageStates[binding].size != size) {
            glBindBufferRange(target, binding, buffer, offset, size);
            mIndexedTargetBindingState.shaderStorageStates[binding].buffer = buffer;
            mIndexedTargetBindingState.shaderStorageStates[binding].offset = offset;
            mIndexedTargetBindingState.shaderStorageStates[binding].size = size;
        }
    }
}
void GLStateManager::PatchInputVertexNum(GLint num) {
    if (mPatchState.inputPatchVertexNum != num) {
        glPatchParameteri(GL_PATCH_VERTICES, num);
        mPatchState.inputPatchVertexNum = num;
    }
}
void GLStateManager::PatchInnerLevels(const std::array<GLfloat, 2> &levels) {
    if (mPatchState.innerLevels != levels) {
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, levels.data());
        mPatchState.innerLevels = levels;
    }
}
void GLStateManager::PatchOuterLevels(const std::array<GLfloat, 4> &levels) {
    if (mPatchState.outerLevels != levels) {
        glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, levels.data());
        mPatchState.outerLevels = levels;
    }
}
}  // namespace Shit