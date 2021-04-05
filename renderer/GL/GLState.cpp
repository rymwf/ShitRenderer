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
namespace Shit
{
	static constexpr GLenum glBufferIndexedTargetArray[]{
		GL_ATOMIC_COUNTER_BUFFER,
		GL_SHADER_STORAGE_BUFFER,
		GL_TRANSFORM_FEEDBACK_BUFFER,
		GL_UNIFORM_BUFFER};

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
	void GLStateManager::Clear()
	{
		//TODO:clear state
	}
	void GLStateManager::BindDrawFramebuffer(GLuint framebuffer)
	{
		if (mDrawBufferState.buffer != framebuffer)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
			mDrawBufferState.buffer = framebuffer;
		}
	}
	void GLStateManager::PushDrawFramebuffer(GLuint framebuffer)
	{
		mDrawBufferState.drawBufferStack.emplace(mDrawBufferState.buffer);
		BindDrawFramebuffer(framebuffer);
	}
	void GLStateManager::PopDrawFramebuffer()
	{
		BindDrawFramebuffer(mDrawBufferState.drawBufferStack.top());
		mDrawBufferState.drawBufferStack.pop();
	}

	void GLStateManager::BindReadFramebuffer(GLuint framebuffer)
	{
		if (mReadBufferState.buffer != framebuffer)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
			mReadBufferState.buffer = framebuffer;
		}
	}
	void GLStateManager::PushReadFramebuffer(GLuint framebuffer)
	{
		mReadBufferState.readBufferStack.emplace(mReadBufferState.buffer);
		BindReadFramebuffer(framebuffer);
	}
	void GLStateManager::PopReadFramebuffer()
	{
		BindReadFramebuffer(mReadBufferState.readBufferStack.top());
		mReadBufferState.readBufferStack.pop();
	}

	void GLStateManager::NotifyReleasedFramebuffer(GLuint framebuffer)
	{
		if (mDrawBufferState.buffer == framebuffer)
			mDrawBufferState.buffer = 0;
		if (mReadBufferState.buffer == framebuffer)
			mReadBufferState.buffer = 0;
	}
	void GLStateManager::BindRenderbuffer(GLuint renderbuffer)
	{
		if (mRenderbufferState.curRenderbuffer != renderbuffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
			mRenderbufferState.curRenderbuffer = renderbuffer;
		}
	}
	void GLStateManager::PushRenderbuffer(GLuint renderbuffer)
	{
		mRenderbufferState.renderbufferStatck.emplace(mRenderbufferState.curRenderbuffer);
		BindRenderbuffer(renderbuffer);
	}
	void GLStateManager::PopRenderbuffer()
	{
		BindRenderbuffer(mRenderbufferState.renderbufferStatck.top());
		mRenderbufferState.renderbufferStatck.pop();
	}
	void GLStateManager::NotifyReleasedRenderbuffer(GLuint renderbuffer)
	{
		if (mRenderbufferState.curRenderbuffer == renderbuffer)
			mRenderbufferState.curRenderbuffer = 0;
	}

	void GLStateManager::BindBuffer(GLenum target, GLuint buffer)
	{
		auto index = glBufferBindTargetMap.at(target);
		if (mBufferState.boundBuffers[index] != buffer)
		{
			glBindBuffer(target, buffer);
			mBufferState.boundBuffers[index] = buffer;
		}
	}
	void GLStateManager::PushBuffer(GLenum target, GLuint buffer)
	{
		auto index = glBufferBindTargetMap.at(target);
		mBufferState.bufferStack.emplace(GLBufferState::StackEntry{target, mBufferState.boundBuffers[index]});
		if (mBufferState.boundBuffers[index] != buffer)
		{
			glBindBuffer(target, buffer);
			mBufferState.boundBuffers[index] = buffer;
		}
	}
	void GLStateManager::PopBuffer()
	{
		auto &&a = mBufferState.bufferStack.top();
		BindBuffer(a.target, a.buffer);
		mBufferState.bufferStack.pop();
	}
	void GLStateManager::NotifyReleaseBuffer(GLuint buffer)
	{
		for (auto &&e : mBufferState.boundBuffers)
		{
			if (e == buffer)
				e = 0;
		}
		for (auto &&e : mIndexedTargetBindingState.uniformBufferStates)
		{
			if (e.buffer == buffer)
				e = {};
		}
		for (auto &&e : mIndexedTargetBindingState.atomicCounterBufferStates)
		{
			if (e.buffer == buffer)
				e = {};
		}
		for (auto &&e : mIndexedTargetBindingState.transformFeedbackBufferStates)
		{
			if (e.buffer == buffer)
				e = {};
		}
		for (auto &&e : mIndexedTargetBindingState.shaderStorageStates)
		{
			if (e.buffer == buffer)
				e = {};
		}
	}
	void GLStateManager::BindVertexArray(GLuint vao)
	{
		if (mPipelineState.vao != vao)
		{
			glBindVertexArray(vao);
			mPipelineState.vao = vao;
		}
	}
	void GLStateManager::NotifyReleasedVertexArray(GLuint vao)
	{
		if (mPipelineState.vao == vao)
			mPipelineState.vao = 0;
	}
	void GLStateManager::BindTextureUnit(GLuint unit, GLenum target, GLuint texture)
	{
		auto &&a = mTextureState.boundTextures[unit];
		if (a.first != target || a.second != texture)
		{
			if (mTextureState.activeUnit != unit)
			{
				glActiveTexture(GL_TEXTURE0 + unit);
				mTextureState.activeUnit = unit;
			}
			glBindTexture(target, texture);
			a = {target, texture};
		}
	}
	void GLStateManager::PushTextureUnit(GLuint unit, GLenum target, GLuint texture)
	{
		auto &&a = mTextureState.boundTextures[unit];
		mTextureState.textureStack.emplace(GLTextureState::StackEntry{unit, a.first, a.second});
		BindTextureUnit(unit, target, texture);
	}
	void GLStateManager::PopTextureUnit()
	{
		auto &&a = mTextureState.textureStack.top();
		if (a.target == 0)
			a.target = mTextureState.boundTextures[a.unit].first;
		BindTextureUnit(a.unit, a.target, a.texture);
		mTextureState.textureStack.pop();
	}
	void GLStateManager::NotifyReleasedTexture(GLuint texture)
	{
		for (auto &&e : mTextureState.boundTextures)
		{
			if (e.second == texture)
				e.second = 0;
		}
		for (auto &&e : mImageState.boundTextures)
		{
			if (e.texture == texture)
				e.texture = 0;
		}
	}
	void GLStateManager::BindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
	{
		auto &&a = mImageState.boundTextures[unit];
		if (a.texture != texture || a.level != level || a.layered != layered || a.layer != layer || a.access != access || a.format != format)
		{
			glBindImageTexture(unit, texture, level, layered, layer, access, format);
			a = {unit, texture, level, layered, layer, access, format};
		}
	}
	void GLStateManager::PushImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format)
	{
		mImageState.imageStack.emplace(mImageState.boundTextures[unit]);
		BindImageTexture(unit, texture, level, layered, layer, access, format);
	}
	void GLStateManager::PopImageTexture()
	{
		auto &&a = mImageState.imageStack.top();
		BindImageTexture(a.unit, a.texture, a.level, a.layered, a.layer, a.access, a.format);
		mImageState.imageStack.pop();
	}
	void GLStateManager::BindSampler(GLuint unit, GLuint sampler)
	{
		if (mSamplerState.boundSamplers[unit] != sampler)
		{
			glBindSampler(unit, sampler);
			mSamplerState.boundSamplers[unit] = sampler;
		}
	}
	void GLStateManager::PushSampler(GLuint unit, GLuint sampler)
	{
		mSamplerState.samplerStack.emplace(GLSamplerState::StackEntry{unit, sampler});
		BindSampler(unit, sampler);
	}
	void GLStateManager::PopSampler()
	{
		auto &&a = mSamplerState.samplerStack.top();
		BindSampler(a.unit, a.sampler);
		mSamplerState.samplerStack.pop();
	}
	void GLStateManager::NotifyReleasedSampler(GLuint sampler)
	{
		for (auto &&e : mSamplerState.boundSamplers)
		{
			if (e == sampler)
				e = 0;
		}
	}
	void GLStateManager::BindPipeline(GLuint pipeline)
	{
		if (mPipelineState.pipeline != pipeline)
		{
			glBindProgramPipeline(pipeline);
			mPipelineState.pipeline = pipeline;
		}
	}
	void GLStateManager::NotifyReleasedPipeline(GLuint pipeline)
	{
		if (mPipelineState.pipeline == pipeline)
		{
			memset(&mPipelineState, 0, sizeof(GLPipelineState));
		}
	}
	void GLStateManager::BindVertexBuffer(GLuint first, GLsizei count,
										  const std::vector<GLuint> &buffers,
										  const std::vector<GLintptr> &offsets,
										  const std::vector<GLsizei> &strides)
	{
		if (
			mPipelineState.vertexBuffer.firstBinding != first ||
			mPipelineState.vertexBuffer.bindingCount != count ||
			mPipelineState.vertexBuffer.buffers != buffers ||
			mPipelineState.vertexBuffer.offsets != offsets)
		{

			glBindVertexBuffers(first,
								count,
								buffers.data(),
								offsets.data(),
								strides.data());
			mPipelineState.vertexBuffer.firstBinding = first;
			mPipelineState.vertexBuffer.bindingCount = count;
			mPipelineState.vertexBuffer.buffers = buffers;
			mPipelineState.vertexBuffer.offsets = offsets;
		}
	}
	void GLStateManager::BindIndexBuffer(GLuint buffer, GLenum indexType, uint64_t offset)
	{
		//TODO: indexbuffer offset
		if (mPipelineState.indexBuffer.buffer != buffer)
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
			mPipelineState.indexBuffer.buffer = buffer;
		}
		mPipelineState.indexBuffer.type = indexType;
		mPipelineState.indexBuffer.offset = offset;
	}
	void GLStateManager::EnableCapability(GLenum cap)
	{
		auto index = glCapabilityTable.at(cap);
		if (!mCapabilityState.capbilityStates[index])
		{
			glEnable(cap);
			mCapabilityState.capbilityStates[index] = true;
		}
	}
	void GLStateManager::DisableCapability(GLenum cap)
	{
		auto index = glCapabilityTable.at(cap);
		if (mCapabilityState.capbilityStates[index])
		{
			glDisable(cap);
			mCapabilityState.capbilityStates[index] = false;
		}
	}
	void GLStateManager::UpdateCapbilityState()
	{
		mCapabilityState.capbilityStates.resize(glCapabilityTable.size());
		for (auto &&e : glCapabilityTable)
		{
			mCapabilityState.capbilityStates[e.second] = glIsEnabled(e.first);
		}
	}
	void GLStateManager::PolygonMode(GLenum mode)
	{
		if (mPolygonState.mode != mode)
		{
			glPolygonMode(GL_FRONT_AND_BACK, mode);
			mPolygonState.mode = mode;
		}
	}
	void GLStateManager::PolygonOffSetClamp(GLfloat factor, GLfloat units, GLfloat clamp)
	{
		if (mPolygonState.offsetFactor != factor || mPolygonState.offsetUnits != units || mPolygonState.clamp != clamp)
		{
			glPolygonOffsetClamp(factor, units, clamp);
			mPolygonState.offsetFactor = factor;
			mPolygonState.offsetUnits = units;
			mPolygonState.clamp = clamp;
		}
	}
	//cull face mode
	void GLStateManager::CullFace(GLenum mode)
	{
		if (mCullModeState.mode != mode)
		{
			glCullFace(mode);
			mCullModeState.mode = mode;
		}
	}

	//front face
	void GLStateManager::FrontFace(GLenum dir)
	{
		if (mFrontFaceState.dir != dir)
		{
			glFrontFace(dir);
			mFrontFaceState.dir = dir;
		}
	}
	void GLStateManager::SetViewports(GLuint first, GLuint count, float *v)
	{
		GLuint i = 0;
		while (first < count)
		{
			if (
				mViewportState.viewports[first].x != v[i * 6 + 0] ||
				mViewportState.viewports[first].y != v[i * 6 + 1] ||
				mViewportState.viewports[first].width != v[i * 6 + 2] ||
				mViewportState.viewports[first].height != v[i * 6 + 3])
			{
				glViewportIndexedf(i, v[first * 6 + 0], v[i * 6 + 1], v[i * 6 + 2], v[i * 6 + 3]);
				mViewportState.viewports[first].x = v[i * 6 + 0];
				mViewportState.viewports[first].y = v[i * 6 + 1];
				mViewportState.viewports[first].width = v[i * 6 + 2];
				mViewportState.viewports[first].height = v[i * 6 + 3];
			}
			if (
				mViewportState.viewports[first].minDepth != v[i * 6 + 4] ||
				mViewportState.viewports[first].maxDepth != v[i * 6 + 5])
			{
				glDepthRangeIndexed(i, v[first * 6 + 4], v[i * 6 + 5]);
				mViewportState.viewports[first].minDepth = v[i * 6 + 4];
				mViewportState.viewports[first].maxDepth = v[i * 6 + 5];
			}
			++first;
			++i;
		}
	}
	void GLStateManager::SetScissors(GLuint first, GLuint count, int32_t *v)
	{
		GLuint i = 0;
		while (first < count)
		{
			//if (!mScissorState.scissors[first].enabled)
			//{
			//	glEnablei(GL_SCISSOR_TEST, first);
			//	mScissorState.scissors[first].enabled = true;
			//}
			if (
				mScissorState.scissors[first].x != v[i * 4 + 0] ||
				mScissorState.scissors[first].y != v[i * 4 + 1] ||
				mScissorState.scissors[first].width != v[i * 4 + 2] ||
				mScissorState.scissors[first].height != v[i * 4 + 3])
			{
				glScissorIndexed(i, v[first * 4 + 0], v[i * 4 + 1], v[i * 4 + 2], v[i * 4 + 3]);
				mScissorState.scissors[first].x = v[i * 4 + 0];
				mScissorState.scissors[first].y = v[i * 4 + 1];
				mScissorState.scissors[first].width = v[i * 4 + 2];
				mScissorState.scissors[first].height = v[i * 4 + 3];
			}
			++first;
			++i;
		}
	}
	void GLStateManager::LineWidth(GLfloat width)
	{
		if (mLineWidth != width)
		{
			glLineWidth(width);
			mLineWidth = width;
		}
	}
	void GLStateManager::StencilOpState(GLenum face, GLenum func, GLint ref, GLuint mask, GLenum sfail, GLenum dpfail, GLenum dppass, GLuint writeMask)
	{
		if (face == GL_FRONT)
		{
			if (
				mStencilOpState.frontOpState.compareOp != func ||
				mStencilOpState.frontOpState.reference != ref ||
				mStencilOpState.frontOpState.compareMask != mask)
			{
				glStencilFuncSeparate(face, func, ref, mask);
				mStencilOpState.frontOpState.compareOp = func;
				mStencilOpState.frontOpState.reference = ref;
				mStencilOpState.frontOpState.compareMask = mask;
			}
			if (
				mStencilOpState.frontOpState.failOp != sfail ||
				mStencilOpState.frontOpState.passOp != dpfail ||
				mStencilOpState.frontOpState.depthFailOp != dpfail)
			{
				glStencilOpSeparate(face, sfail, dpfail, dppass);
				mStencilOpState.frontOpState.failOp = sfail;
				mStencilOpState.frontOpState.passOp = dpfail;
				mStencilOpState.frontOpState.depthFailOp = dpfail;
			}
			if (mStencilOpState.frontOpState.writeMask != writeMask)
			{
				glStencilMaskSeparate(face, writeMask);
				mStencilOpState.frontOpState.writeMask = writeMask;
			}
		}
		else if (face == GL_BACK)
		{
			if (
				mStencilOpState.backOpState.compareOp != func ||
				mStencilOpState.backOpState.reference != ref ||
				mStencilOpState.backOpState.compareMask != mask)
			{
				glStencilFuncSeparate(face, func, ref, mask);
				mStencilOpState.backOpState.compareOp = func;
				mStencilOpState.backOpState.reference = ref;
				mStencilOpState.backOpState.compareMask = mask;
			}
			if (
				mStencilOpState.backOpState.failOp != sfail ||
				mStencilOpState.backOpState.passOp != dpfail ||
				mStencilOpState.backOpState.depthFailOp != dpfail)
			{
				glStencilOpSeparate(face, sfail, dpfail, dppass);
				mStencilOpState.backOpState.failOp = sfail;
				mStencilOpState.backOpState.passOp = dpfail;
				mStencilOpState.backOpState.depthFailOp = dpfail;
			}
			if (mStencilOpState.backOpState.writeMask != writeMask)
			{
				glStencilMaskSeparate(face, writeMask);
				mStencilOpState.backOpState.writeMask = writeMask;
			}
		}
		else if (face == GL_FRONT_AND_BACK)
		{
			StencilOpState(GL_FRONT, func, ref, mask, sfail, dpfail, dppass, writeMask);
			StencilOpState(GL_BACK, func, ref, mask, sfail, dpfail, dppass, writeMask);
		}
	}
	void GLStateManager::DepthFunc(GLenum func)
	{
		if (mDepthTestState.compareFunc != func)
		{
			glDepthFunc(func);
			mDepthTestState.compareFunc = func;
		}
	}
	void GLStateManager::DepthMask(bool enable)
	{
		if (mDepthTestState.writeEnable != enable)
		{
			glDepthMask(enable);
			mDepthTestState.writeEnable = enable;
		}
	}
	void GLStateManager::MinSampleShading(GLfloat value)
	{
		if (mMultisampleState.minSampleShading != value)
		{
			glMinSampleShading(value);
			mMultisampleState.minSampleShading = value;
		}
	}
	void GLStateManager::BlendColor(const std::array<float, 4> &color)
	{
		if (mBlendState.blendColor != color)
		{
			glBlendColor(color[0], color[1], color[2], color[3]);
			mBlendState.blendColor = color;
		}
	}
	void GLStateManager::ColorMask(GLuint index, GLuint mask)
	{
		if (mBlendState.attachmentBlendStates[index].writeMask != mask)
		{
			glColorMaski(index, mask & 0x1, mask & 0x2, mask & 0x4, mask & 0x8);
			mBlendState.attachmentBlendStates[index].writeMask = mask;
		}
	}
	void GLStateManager::EnableBlend(GLuint index)
	{
		if (!mBlendState.attachmentBlendStates[index].enable)
		{
			glEnablei(GL_BLEND, index);
			mBlendState.attachmentBlendStates[index].enable = true;
		}
	}
	void GLStateManager::DisableBlend(GLuint index)
	{
		if (mBlendState.attachmentBlendStates[index].enable)
		{
			glDisablei(GL_BLEND, index);
			mBlendState.attachmentBlendStates[index].enable = false;
		}
	}
	void GLStateManager::BlendEquation(GLuint index, GLenum modeRGB, GLenum modeAlpha)
	{
		if (
			mBlendState.attachmentBlendStates[index].modeRGB != modeRGB ||
			mBlendState.attachmentBlendStates[index].modeAlpha != modeAlpha)
		{
			glBlendEquationSeparatei(index, modeRGB, modeAlpha);
			mBlendState.attachmentBlendStates[index].modeRGB = modeRGB;
			mBlendState.attachmentBlendStates[index].modeAlpha = modeAlpha;
		}
	}
	void GLStateManager::BlendFunc(GLuint index, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
	{
		if (
			mBlendState.attachmentBlendStates[index].srcRGB != srcRGB ||
			mBlendState.attachmentBlendStates[index].dstRGB != dstRGB ||
			mBlendState.attachmentBlendStates[index].srcAlpha != srcAlpha ||
			mBlendState.attachmentBlendStates[index].dstAlpha != dstAlpha)
		{
			glBlendFuncSeparatei(index, srcRGB, dstRGB, srcAlpha, dstAlpha);
			mBlendState.attachmentBlendStates[index].srcRGB = srcRGB;
			mBlendState.attachmentBlendStates[index].dstRGB = dstRGB;
			mBlendState.attachmentBlendStates[index].srcAlpha = srcAlpha;
			mBlendState.attachmentBlendStates[index].dstAlpha = dstAlpha;
		}
	}
	void GLStateManager::LogicOp(GLenum op)
	{
		if (mBlendState.logicOp != op)
		{
			glLogicOp(op);
			mBlendState.logicOp = op;
		}
	}
	void GLStateManager::EnablePrimitiveRestart()
	{
		if (!mAssemblyState.enable)
		{
			glEnable(GL_PRIMITIVE_RESTART);
			mAssemblyState.enable = true;
		}
	}
	void GLStateManager::DisablePrimitiveRestart()
	{
		if (mAssemblyState.enable)
		{
			glDisable(GL_PRIMITIVE_RESTART);
			mAssemblyState.enable = false;
		}
	}
	void GLStateManager::PrimitiveTopology(GLenum topology)
	{
		mAssemblyState.primitiveTopology = topology;
	}
	void GLStateManager::ClipControl(GLenum origin, GLenum depth)
	{
		if (mClipState.origin != origin || mClipState.depth != depth)
		{
			glClipControl(origin, depth);
			mClipState.origin = origin;
			mClipState.depth = depth;
		}
	}
	void GLStateManager::BindBufferRange(GLenum target, GLuint binding, GLuint buffer, GLintptr offset, GLintptr size)
	{
		if (target == GL_UNIFORM_BUFFER)
		{
			if (
				mIndexedTargetBindingState.uniformBufferStates[binding].buffer != buffer ||
				mIndexedTargetBindingState.uniformBufferStates[binding].offset != offset ||
				mIndexedTargetBindingState.uniformBufferStates[binding].size != size)
			{
				glBindBufferRange(target, binding, buffer, offset, size);
				mIndexedTargetBindingState.uniformBufferStates[binding].buffer = buffer;
				mIndexedTargetBindingState.uniformBufferStates[binding].offset = offset;
				mIndexedTargetBindingState.uniformBufferStates[binding].size = size;
			}
		}
		else if (target == GL_ATOMIC_COUNTER_BUFFER)
		{
			if (
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].buffer != buffer ||
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].offset != offset ||
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].size != size)
			{
				glBindBufferRange(target, binding, buffer, offset, size);
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].buffer = buffer;
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].offset = offset;
				mIndexedTargetBindingState.atomicCounterBufferStates[binding].size = size;
			}
		}
		else if (target == GL_TRANSFORM_FEEDBACK_BUFFER)
		{
			if (
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].buffer != buffer ||
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].offset != offset ||
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].size != size)
			{
				glBindBufferRange(target, binding, buffer, offset, size);
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].buffer = buffer;
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].offset = offset;
				mIndexedTargetBindingState.transformFeedbackBufferStates[binding].size = size;
			}
		}
		else if (target == GL_SHADER_STORAGE_BUFFER)
		{
			if (
				mIndexedTargetBindingState.shaderStorageStates[binding].buffer != buffer ||
				mIndexedTargetBindingState.shaderStorageStates[binding].offset != offset ||
				mIndexedTargetBindingState.shaderStorageStates[binding].size != size)
			{
				glBindBufferRange(target, binding, buffer, offset, size);
				mIndexedTargetBindingState.shaderStorageStates[binding].buffer = buffer;
				mIndexedTargetBindingState.shaderStorageStates[binding].offset = offset;
				mIndexedTargetBindingState.shaderStorageStates[binding].size = size;
			}
		}
	}
}