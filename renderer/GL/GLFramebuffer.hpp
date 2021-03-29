/**
 * @file GLFramebuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitFramebuffer.hpp>
#include "GLPrerequisites.hpp"
namespace Shit
{
	class GLFramebuffer final : public Framebuffer
	{
		GLuint mRenderFBO{~0u};
		GLuint mResolveFBO{~0u};
		GLStateManager *mpStateManager;

		GLenum mReadBuffer{};
		std::vector<GLenum> mDrawBuffers{};

		uint32_t mCurRenderFBOSubpassIndex{~0u};
		uint32_t mCurResolveFBOSubpassIndex{~0u};

		static void CheckCompleteness();

	public:
		GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo);
		~GLFramebuffer()
		{
			mpStateManager->NotifyReleasedFramebuffer(mRenderFBO);
			glDeleteFramebuffers(1, &mRenderFBO);
			mpStateManager->NotifyReleasedFramebuffer(mResolveFBO);
			glDeleteFramebuffers(1, &mResolveFBO);
		}

		void SetRenderFBOAttachment(uint32_t subpassIndex);
		void SetResolveFBOAttachment(uint32_t subpassIndex);

		void Resolve(Filter filter);

		constexpr GLuint GetHandle() const
		{
			return mResolveFBO == (~0U) ? mRenderFBO : mResolveFBO;
		}
		constexpr GLuint GetRenderFramebuffer() const
		{
			return mRenderFBO;
		}
		constexpr GLuint GetResolveFramebuffer() const
		{
			return mResolveFBO;
		}
		void BindReadBuffer(GLenum index);
		void BindDrawBuffers(const std::vector<GLenum> &indices);
	};
} // namespace Shit
