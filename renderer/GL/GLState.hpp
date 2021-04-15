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
#include <stack>
#include <array>
#include <vector>
#include <unordered_map>

#define BUFFER_TARGET_NUM 15

#define MAX_DRAW_BUFFERS 8

#define MAX_VIEWPORTS 16
#define MAX_VERTEX_ATTRIB_BINDINGS 16

//min value of vertex shader limist
#define MAX_VERTEX_ATTRIBS 16
#define MAX_VERTEX_UNIFORM_VECTORS 256
#define MAX_VERTEX_UNIFORM_BLOCKS 14
#define MAX_VERTEX_TEXTURE_IMAGE_UNITS 16

//min value of tesselation shader limist

//min value of geometry shader limist
#define MAX_GEOMETRY_UNIFORM_BLOCKS 14
#define MAX_GEOMETRY_TEXTURE_IMAGE_UNITS 16
#define MAX_GEOMETRY_SHADER_INVOCATIONS 32
#define MAX_VERTEX_STREAMS 4

//min value of fragment shader limist
#define MAX_FRAGMENT_UNIFORM_BLOCKS 14
#define MAX_TEXTURE_IMAGE_UNITS 16
#define MAX_FRAGMENT_ATOMIC_COUNTER_BUFFERS 1
#define MAX_FRAGMENT_ATOMIC_COUNTERS 8
#define MAX_FRAGMENT_SHADER_STORAGE_BLOCKS 8

//min value of compute shader limist 
#define MAX_COMPUTE_UNIFORM_BLOCKS 14
#define MAX_COMPUTE_TEXTURE_IMAGE_UNITS 16
#define MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS 8
#define MAX_COMPUTE_ATOMIC_COUNTERS 8
#define MAX_COMPUTE_IMAGE_UNIFORMS 8
#define MAX_COMPUTE_SHADER_STORAGE_BLOCKS 8

//min value of aggregate shader limits
#define MAX_UNIFORM_BUFFER_BINDING 84
#define MAX_COMBINED_UNIFORM_BLOCKS 70
#define MAX_COMBINED_TEXTURE_IMAGE_UNITS 80
#define MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 1
#define MAX_COMBINED_ATOMIC_COUNTER_BUFFERS 1
#define MAX_COMBINED_ATOMIC_COUNTERS 8
#define MAX_SHADER_STORAGE_BUFFER_BINDINGS 8
#define MAX_COMBINED_SHADER_STORAGE_BLOCKS 8
#define MAX_IMAGE_UNITS 8
#define MAX_COMBINED_SHADER_OUTPUT_RESOURCES 8
#define MAX_FRAGMENT_IMAGE_UNIFORMS 8
#define MAX_COMBINED_IMAGE_UNIFORMS 8

//
#define MAX_TRANSFORM_FEEDBACK_BUFFERS 4

// self defined var
#define MAX_UNIFORM_BLOCKS 32 
#define MAX_IMAGE_UNIFORMS 8

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
			std::array<std::pair<GLenum, GLuint>, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundTextures;
			std::stack<StackEntry> textureStack;
		} mTextureState;

		struct GLSamplerState
		{
			struct StackEntry
			{
				GLuint unit;
				GLuint sampler;
			};
			std::array<GLuint, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundSamplers{};
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
			std::array<StackEntry, MAX_COMBINED_TEXTURE_IMAGE_UNITS> boundTextures;
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

		struct GLPipelineState
		{
			struct VertexBuffer
			{
				GLuint firstBinding;
				GLsizei bindingCount;
				std::vector<GLuint> buffers;
				std::vector<GLintptr> offsets;
			} vertexBuffer;
			struct IndexBufferState
			{
				GLuint buffer;
				GLenum type;
				uint64_t offset;
			} indexBuffer;
			GLuint pipeline;
			GLuint vao;
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

		struct ClipState
		{
			GLenum origin;
			GLenum depth;
		} mClipState;

		struct IndexedTargetBindingState
		{
			struct Entry
			{
				GLuint buffer;
				GLintptr offset;
				GLsizeiptr size;
			};
			std::array<Entry, MAX_UNIFORM_BLOCKS> uniformBufferStates;
			std::array<Entry, MAX_COMBINED_ATOMIC_COUNTERS> atomicCounterBufferStates;
			std::array<Entry, MAX_TRANSFORM_FEEDBACK_BUFFERS> transformFeedbackBufferStates;
			std::array<Entry, MAX_COMBINED_SHADER_STORAGE_BLOCKS> shaderStorageStates;
		} mIndexedTargetBindingState;

		struct PatchState
		{
			GLint inputPatchVertexNum{};
			std::array<GLfloat, 4> outerLevels{};
			std::array<GLfloat, 2> innerLevels{};
		} mPatchState;

	public:
		GLStateManager() = default;
		void Clear();

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
		void NotifyReleasedVertexArray(GLuint vao);

		//pipeline
		void BindPipeline(GLuint pipeline);
		void NotifyReleasedPipeline(GLuint pipeline);
		void BindVertexBuffer(GLuint first, GLsizei count,
							  const std::vector<GLuint> &buffers,
							  const std::vector<GLintptr> &offsets,
							  const std::vector<GLsizei> &strides);
		void BindIndexBuffer(GLuint buffer, GLenum indexType, uint64_t offset);
		constexpr GLenum GetIndexType()
		{
			return mPipelineState.indexBuffer.type;
		}
		constexpr uint64_t GetIndexOffset()
		{
			return mPipelineState.indexBuffer.offset;
		}

		//texture
		void BindTextureUnit(GLuint unit, GLenum target, GLuint texture);
		void PushTextureUnit(GLuint unit, GLenum target, GLuint texture);
		void PopTextureUnit();
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

		//clip
		void ClipControl(GLenum origin, GLenum depth);

		void BindBufferRange(GLenum target, GLuint binding, GLuint buffer, GLintptr offset, GLintptr size);

		void PatchInputVertexNum(GLint num);
		void PatchInnerLevels(const std::array<GLfloat, 2> &levels);
		void PatchOuterLevels(const std::array<GLfloat, 4> &levels);
	};
} // namespace Shit
