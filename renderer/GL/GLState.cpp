/**
 * @file GLState.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLState.h"
namespace Shit
{
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

	void GLStateManager::BindDrawFramebuffer(GLuint framebuffer)
	{
		if (mDrawBufferState.curDrawBuffer != framebuffer)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
			mDrawBufferState.curDrawBuffer = framebuffer;
		}
	}
	void GLStateManager::PushDrawFramebuffer(GLuint framebuffer)
	{
		mDrawBufferState.drawBufferStack.emplace(mDrawBufferState.curDrawBuffer);
		BindDrawFramebuffer(framebuffer);
	}
	void GLStateManager::PopDrawFramebuffer()
	{
		BindDrawFramebuffer(mDrawBufferState.drawBufferStack.top());
		mDrawBufferState.drawBufferStack.pop();
	}

	void GLStateManager::BindReadFramebuffer(GLuint framebuffer)
	{
		if (mReadBufferState.curReadBuffer != framebuffer)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
			mReadBufferState.curReadBuffer = framebuffer;
		}
	}
	void GLStateManager::PushReadFramebuffer(GLuint framebuffer)
	{
		mReadBufferState.readBufferStack.emplace(mReadBufferState.curReadBuffer);
		BindReadFramebuffer(framebuffer);
	}
	void GLStateManager::PopReadFramebuffer()
	{
		BindReadFramebuffer(mReadBufferState.readBufferStack.top());
		mReadBufferState.readBufferStack.pop();
	}

	void GLStateManager::NotifyReleasedFramebuffer(GLuint framebuffer)
	{
		if (mDrawBufferState.curDrawBuffer == framebuffer)
			mDrawBufferState.curDrawBuffer = 0;
		if (mReadBufferState.curReadBuffer == framebuffer)
			mReadBufferState.curReadBuffer = 0;
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
		auto a = mBufferState.bufferStack.top();
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
	}
	void GLStateManager::BindVertexArray(GLuint vao)
	{
		if (mVAOState.curVAO != vao)
		{
			glBindVertexArray(vao);
			mVAOState.curVAO = vao;
		}
	}
	void GLStateManager::PushVertexArray(GLuint vao)
	{
		mVAOState.vaoStack.emplace(mVAOState.curVAO);
		BindVertexArray(vao);
	}
	void GLStateManager::PopVertexArray()
	{
		BindVertexArray(mVAOState.vaoStack.top());
		mVAOState.vaoStack.pop();
	}
	void GLStateManager::NotifyReleasedVertexArray(GLuint vao)
	{
		if (mVAOState.curVAO == vao)
			mVAOState.curVAO = 0;
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
	void GLStateManager::PushTexture(GLuint unit, GLenum target, GLuint texture)
	{
		auto &&a = mTextureState.boundTextures[unit];
		mTextureState.textureStack.emplace(GLTextureState::StackEntry{unit, a.first, a.second});
		BindTextureUnit(unit, target, texture);
	}
	void GLStateManager::PopTexture()
	{
		auto &&a = mTextureState.textureStack.top();
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
		if (mPipelineState.curPipeline != pipeline)
		{
			glBindProgramPipeline(pipeline);
			mPipelineState.curPipeline = pipeline;
		}
	}
	void GLStateManager::PushPipeline(GLuint pipeline)
	{
		mPipelineState.pipelineStack.emplace(mPipelineState.curPipeline);
		BindPipeline(pipeline);
	}
	void GLStateManager::PopPipeline()
	{
		BindPipeline(mPipelineState.pipelineStack.top());
		mPipelineState.pipelineStack.pop();
	}
	void GLStateManager::NotifyReleasedPipeline(GLuint pipeline)
	{
		if (mPipelineState.curPipeline == pipeline)
			mPipelineState.curPipeline = 0;
	}
}