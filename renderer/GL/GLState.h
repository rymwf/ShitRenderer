/**
 * @file GLState.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "GLPrerequisites.h"
#include <renderer/ShitNonCopyable.h>
namespace Shit
{
	class GLStateManager : public NonCopyable
	{
		struct GLDrawBufferState
		{
			GLuint framebuffer;
			GLuint curDrawBuffer;
			std::stack<GLuint> drawBufferStack;
		} mDrawBufferState;

		struct GLReadBufferState
		{
			GLuint framebuffer;
			GLuint curReadBuffer;
			std::stack<GLuint> readBufferStack;
		} mReadBufferState;

		struct GLTextureState
		{
			struct StackEntry
			{
				GLuint unit; //!< index of textureUnits	array, previous active unit
				GLenum target;
				GLuint texture;
			};
			GLuint activeUnit;
			//target and texture
			std::array<std::pair<GLenum, GLuint>, MAX_TEXTURE_IMAGE_UNITS> boundTextures;
			std::stack<StackEntry> textureStack;
		} mTextureState;

		struct GLSamplerState
		{
			struct StackEntry
			{
				GLuint unit;
				GLuint sampler;
			};
			std::array<GLuint, MAX_TEXTURE_IMAGE_UNITS> boundSamplers{};
			std::stack<StackEntry> samplerStack;
		} mSamplerState;
		struct GLImageUnitState
		{
			struct StackEntry
			{
				GLuint unit;
				GLuint texture;
				GLint level;
				GLboolean layered;
				GLint layer;
				GLenum access;
				GLenum format;
			};
			std::array<StackEntry, MAX_TEXTURE_IMAGE_UNITS> boundTextures;
			std::stack<StackEntry> imageStack;
		} mImageState;
		struct GLRenderbufferState
		{
			GLuint curRenderbuffer;
			std::stack<GLuint> renderbufferStatck;
		} mRenderbufferState;
		struct GLBufferState
		{
			struct StackEntry
			{
				GLenum target;
				GLuint buffer;
			};
			std::array<GLuint, BUFFER_TARGET_NUM> boundBuffers;
			std::stack<StackEntry> bufferStack;
		} mBufferState;
		struct GLVertexArrayState
		{
			GLuint curVAO;
			std::stack<GLuint> vaoStack;
		} mVAOState;

		//		struct ColorMaskState
		//		{
		//			uint32_t colorBitMask;
		//			std::stack<int> maskStack;
		//		} mColorMaskState;
		//		struct DepthMaskState
		//		{
		//			bool curMask;
		//			std::stack<bool> maskStack;
		//		} mDepthMaskState;
		//		struct StencilMaskState
		//		{
		//			struct StackEntry
		//			{
		//				uint32_t backfaceBitmask;
		//				uint32_t frontfaceBitmask;
		//			};
		//			uint32_t backfaceBitmask;
		//			uint32_t frontfaceBitmask;
		//			std::stack<StackEntry> maskStack;
		//		} mStencilState;

		struct PipelineState
		{
			GLuint curPipeline;
			std::stack<GLuint> pipelineStack;
		} mPipelineState;

	public:
		GLStateManager() = default;

		//framebuffer
		void BindDrawBuffer(GLuint framebuffer);
		void PushDrawBuffer(GLuint framebuffer);
		void PopDrawBuffer();
		void BindReadBuffer(GLuint framebuffer);
		void PushReadBuffer(GLuint framebuffer);
		void PopReadBuffer();
		void NotifyReleasedFramebuffer(GLuint framebuffer);

		//renderbuffer
		void BindRenderbuffer(GLuint renderbuffer);
		void PushRenderbuffer(GLuint renderbuffer);
		void PopRenderbuffer();
		void NotifyReleasedRenderbuffer(GLuint renderbuffer);

		//buffer
		void BindBuffer(GLenum target, GLuint buffer);
		void PushBuffer(GLenum target, GLuint buffer);
		void PopBuffer();
		void NotifyReleaseBuffer(GLuint buffer);

		//vao
		void BindVAO(GLuint vao);
		void PushVAO(GLuint vao);
		void PopVAO();
		void NotifyReleasedVAO(GLuint vao);

		//texture
		void BindTextureUnit(GLuint unit, GLenum target, GLuint texture);
		void PushTexture(GLuint unit, GLenum target, GLuint texture);
		void PopTexture();
		void NotifyReleasedTexture(GLuint texture);

		//image
		void BindImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
		void PushImageTexture(GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
		void PopImageTexture();

		//sampler
		void BindSampler(GLuint unit, GLuint sampler);
		void PushSampler(GLuint unit, GLuint sampler);
		void PopSampler();
		void NotifyReleasedSampler(GLuint sampler);

		//pipeline
		void BindPipeline(GLuint pipeline);
		void PushPipeline(GLuint pipeline);
		void PopPipeline();
		void NotifyReleasedPipeline(GLuint pipeline);
	};
} // namespace Shit
