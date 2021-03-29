/**
 * @file GLFramebuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLFramebuffer.h"
#include "GLImage.h"
#include "GLRenderPass.h"
namespace Shit
{
	void GLFramebuffer::CheckCompleteness()
	{
		auto status = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
			THROW("framebuffe error code:" + std::to_string(status));
	}
	void GLFramebuffer::SetRenderFBOAttachment(uint32_t subpassIndex)
	{
		if (mCurRenderFBOSubpassIndex == subpassIndex)
			return;
		mCurRenderFBOSubpassIndex = subpassIndex;
		mpStateManager->BindDrawFramebuffer(mRenderFBO);
		//color attachments
		uint32_t i = 0;
		std::vector<GLenum> bindpoints;
		for (auto &&e : mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpasses[subpassIndex].colorAttachments)
		{
			auto pImageView = static_cast<GLImageView *>(mCreateInfo.attachments[e.attachment]);
			GLenum bindpoint = GL_COLOR_ATTACHMENT0 + i;
			if (static_cast<GLImage *>(pImageView->GetCreateInfoPtr()->pImage)->IsRenderbuffer())
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindpoint, GL_RENDERBUFFER, pImageView->GetHandle());
			else
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindpoint, pImageView->GetHandle(), 0);
			bindpoints.emplace_back(bindpoint);
			++i;
		}
		BindDrawBuffers(bindpoints);

		//depth stencil attachment
		auto depthAttachment = mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpasses[subpassIndex].depthStencilAttachment;
		if (depthAttachment.has_value())
		{
			auto pDepthImageView = static_cast<GLImageView *>(mCreateInfo.attachments[depthAttachment->attachment]);
			GLenum bindpoint{};
			auto externalFormat = MapExternalFormat(pDepthImageView->GetCreateInfoPtr()->format);
			if (externalFormat == GL_DEPTH_STENCIL)
				bindpoint = GL_DEPTH_STENCIL_ATTACHMENT;
			else if (externalFormat == GL_DEPTH_COMPONENT)
				bindpoint = GL_DEPTH_ATTACHMENT;
			else if (externalFormat == GL_STENCIL_INDEX)
				bindpoint = GL_STENCIL_ATTACHMENT;
			if (static_cast<GLImage *>(pDepthImageView->GetCreateInfoPtr()->pImage)->IsRenderbuffer())
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindpoint, GL_RENDERBUFFER, pDepthImageView->GetHandle());
			else
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindpoint, pDepthImageView->GetHandle(), 0);
		}
	}
	void GLFramebuffer::SetResolveFBOAttachment(uint32_t subpassIndex)
	{
		if (mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpasses[subpassIndex].resolveAttachments.size() == 0 || mCurResolveFBOSubpassIndex == subpassIndex)
			return;
		mCurResolveFBOSubpassIndex = subpassIndex;
		if (mResolveFBO == (~0U))
			glGenFramebuffers(1, &mResolveFBO);
		mpStateManager->BindDrawFramebuffer(mResolveFBO);
		//color attachments
		uint32_t i = 0;
		std::vector<GLenum> bindpoints;
		for (auto &&e : mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpasses[subpassIndex].resolveAttachments)
		{
			auto pImageView = static_cast<GLImageView *>(mCreateInfo.attachments[e.attachment]);
			GLenum bindpoint = GL_COLOR_ATTACHMENT0 + i;
			if (static_cast<GLImage *>(pImageView->GetCreateInfoPtr()->pImage)->IsRenderbuffer())
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, bindpoint, GL_RENDERBUFFER, pImageView->GetHandle());
			else
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, bindpoint, pImageView->GetHandle(), 0);
			bindpoints.emplace_back(bindpoint);
			++i;
		}
		BindDrawBuffers(bindpoints);
	}
	void GLFramebuffer::Resolve(Filter filter)
	{
		if (mRenderFBO == (~0U))
			return;
		mpStateManager->BindReadFramebuffer(mRenderFBO);
		mpStateManager->BindDrawFramebuffer(mResolveFBO);
		SetResolveFBOAttachment(mCurRenderFBOSubpassIndex);
		for (uint32_t i = 0, len = static_cast<uint32_t>(mCreateInfo.pRenderPass->GetCreateInfoPtr()->subpasses[mCurRenderFBOSubpassIndex].resolveAttachments.size());
			 i < len; ++i)
		{
			BindReadBuffer(GL_COLOR_ATTACHMENT0 + i);
			BindDrawBuffers({GL_COLOR_ATTACHMENT0 + i});
			glBlitFramebuffer(
				0, 0, mCreateInfo.extent.width, mCreateInfo.extent.height,
				0, 0, mCreateInfo.extent.width, mCreateInfo.extent.height,
				GL_COLOR_BUFFER_BIT, Map(filter));
		}
	}
	GLFramebuffer::GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo)
		: Framebuffer(createInfo), mpStateManager(pStateManager)
	{
		glGenFramebuffers(1, &mRenderFBO);
	}
	void GLFramebuffer::BindReadBuffer(GLenum index)
	{
		if (mReadBuffer != index)
		{
			glReadBuffer(index);
			mReadBuffer = index;
		}
	}
	void GLFramebuffer::BindDrawBuffers(const std::vector<GLenum> &indices)
	{
		if (mDrawBuffers != indices)
		{
			glDrawBuffers(static_cast<GLsizei>(indices.size()), indices.data());
			mDrawBuffers = indices;
		}
	}
} // namespace Shit
