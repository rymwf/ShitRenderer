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
#define MAX_VIEWPORTS 16
#define MAX_DRAW_BUFFERS 8
namespace Shit
{
	class GLStateManager
	{
		struct GLDrawBufferState
		{
			GLuint buffer;
			std::stack<GLuint> drawBufferStack;
		} mDrawBufferState;

		struct GLReadBufferState
		{
			GLuint buffer;
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
		} mVAOState;

		struct GLPipelineState
		{
			GLuint curPipeline;
		} mPipelineState;

		struct GLCapbilityState
		{
			std::vector<bool> capbilityStates;
		} mCapabilityState;

		struct GLPolygonState
		{
			GLenum mode;
			GLfloat offsetFactor;
			GLfloat offsetUnits;
			GLfloat clamp;
		} mPolygonState;

		struct GLCullModeState
		{
			GLenum mode;
		} mCullModeState;
		struct GLFrontFaceState
		{
			GLenum dir;
		} mFrontFaceState;

		struct ViewportState
		{
			struct Viewport
			{
				GLfloat x;
				GLfloat y;
				GLfloat width;
				GLfloat height;
				GLfloat minDepth;
				GLfloat maxDepth;
			};
			std::array<Viewport, MAX_VIEWPORTS> viewports{};
		} mViewportState;

		struct ScissorState
		{
			struct Scissor
			{
				//bool enabled;
				GLint x;
				GLint y;
				GLint width;
				GLint height;
			};
			std::array<Scissor, MAX_VIEWPORTS> scissors{};
		} mScissorState;

		GLfloat mLineWidth;

		struct MultisampleState
		{
			GLfloat minSampleShading;
		} mMultisampleState;

		struct StencilOpState
		{
			struct Op
			{
				GLenum failOp;		//stencil fail
				GLenum passOp;		//both pass
				GLenum depthFailOp; //stencil pass depth fail
				GLenum compareOp;
				GLuint compareMask;
				GLuint writeMask;
				GLint reference;
			};
			Op frontOpState;
			Op backOpState;
		} mStencilOpState;

		struct DepthTestState
		{
			bool writeEnable;
			GLenum compareFunc;
		} mDepthTestState;

		struct BlendState
		{
			struct AttachmentBlendState
			{
				bool enable;
				GLenum modeRGB;
				GLenum modeAlpha;
				GLenum srcRGB;
				GLenum srcAlpha;
				GLenum dstRGB;
				GLenum dstAlpha;
				GLuint writeMask;
			};
			std::array<AttachmentBlendState, MAX_DRAW_BUFFERS> attachmentBlendStates;
			GLenum logicOp;
			std::array<float, 4> blendColor;
		} mBlendState;

		struct AssemblyState
		{
			bool enable;
			GLenum primitiveTopology;
		} mAssemblyState;

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
		void NotifyReleasedPipeline(GLuint pipeline);

		//capbility state
		void EnableCapability(GLenum cap);
		void DisableCapability(GLenum cap);
		void UpdateCapbilityState();

		//polygon mode
		void PolygonMode(GLenum mode);
		void PolygonOffSetClamp(GLfloat factor, GLfloat units, GLfloat clamp);

		//cull face mode
		void CullFace(GLenum mode);

		//front face
		void FrontFace(GLenum dir);

		/**
		 * @brief Set the Viewports object
		 * 
		 * @param first 
		 * @param count 
		 * @param v x,y,w,h,minDepth,maxDepth
		 */
		void SetViewports(GLuint first, GLuint count, float *v);

		/**
		 * @brief Set the Scissors object
		 * 
		 * @param first 
		 * @param count 
		 * @param v x,y, width,height
		 */
		void SetScissors(GLuint first, GLuint count, int32_t *v);

		void LineWidth(GLfloat width);

		void StencilOpState(GLenum face, GLenum func, GLint ref, GLuint mask, GLenum sfail, GLenum dpfail, GLenum dppass, GLuint writeMask);

		void DepthFunc(GLenum func);
		void DepthMask(bool enable);

		void MinSampleShading(GLfloat value);

		void BlendColor(const std::array<float, 4> &color);

		/**
		 * @brief 
		 * 
		 * @param index 
		 * @param mask 0x1 red, 0x2 green, 0x4 blue, 0x8 alpha
		 */
		void ColorMask(GLuint index, GLuint mask);

		void EnableBlend(GLuint index);
		void DisableBlend(GLuint index);
		void BlendEquation(GLuint index, GLenum modeRGB, GLenum modeAlpha);
		void BlendFunc(GLuint index, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
		void LogicOp(GLenum op);

		void EnablePrimitiveRestart();
		void DisablePrimitiveRestart();
		void PrimitiveTopology(GLenum topology);
		constexpr GLenum GetPrimitiveTopology()
		{
			return mAssemblyState.primitiveTopology;
		}
	};
} // namespace Shit
