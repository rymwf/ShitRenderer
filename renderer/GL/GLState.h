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
#include <GL/glew.h>
#include <stack>
#include <array>
#include <vector>
#include <unordered_map>

#define MAX_TEXTURE_IMAGE_UNITS 80 //!< The number of texture units is implementation dependent, but must be at least 80, the value can be get from GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS
#define BUFFER_TARGET_NUM 15
namespace Shit
{
	class GLStateManager
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

		struct GLPipelineState
		{
			GLuint curPipeline;
			std::stack<GLuint> pipelineStack;
		} mPipelineState;
	public:
		GLStateManager() = default;

		//framebuffer
		void BindDrawFramebuffer(GLuint framebuffer);
		void PushDrawFramebuffer(GLuint framebuffer);
		void PopDrawFramebuffer();
		void BindReadFramebuffer(GLuint framebuffer);
		void PushReadFramebuffer(GLuint framebuffer);
		void PopReadFramebuffer();
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
		void BindVertexArray(GLuint vao);
		void PushVertexArray(GLuint vao);
		void PopVertexArray();
		void NotifyReleasedVertexArray(GLuint vao);

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
