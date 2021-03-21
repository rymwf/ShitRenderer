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
namespace Shit
{

	GLFramebuffer::GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo)
		: Framebuffer(createInfo), mpStateManager(pStateManager)
	{
		glGenFramebuffers(1, &mHandle);
		mpStateManager->PushDrawFramebuffer(mHandle);
		int i = 0;
		auto attachment = GL_COLOR_ATTACHMENT0;
		std::vector<uint32_t> colorAttachmentIndices;
		colorAttachmentIndices.reserve(mCreateInfo.attachments.size());
		for (auto &&pImageView : mCreateInfo.attachments)
		{
			auto externalFormat = MapExternalFormat(pImageView->GetCreateInfoPtr()->format);
			if (externalFormat == GL_DEPTH_STENCIL)
				attachment = GL_DEPTH_STENCIL_ATTACHMENT;
			else if (externalFormat == GL_DEPTH_COMPONENT)
				attachment = GL_DEPTH_ATTACHMENT;
			else if (externalFormat == GL_STENCIL_INDEX)
				attachment = GL_STENCIL_ATTACHMENT;
			else
			{
				attachment = GL_COLOR_ATTACHMENT0 + i;
				colorAttachmentIndices.emplace_back(i);
				++i;
			}
			if (static_cast<GLImage *>(pImageView->GetCreateInfoPtr()->pImage)->IsRenderbuffer())
				glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment, GL_RENDERBUFFER, static_cast<GLImageView *>(pImageView)->GetHandle());
			else
				glFramebufferTexture(GL_DRAW_FRAMEBUFFER, attachment, static_cast<GLImageView *>(pImageView)->GetHandle(), 0);
		}
		BindDrawBuffers(colorAttachmentIndices);
		mpStateManager->PopDrawFramebuffer();
	}
	void GLFramebuffer::BindReadBuffer(uint32_t index)
	{
		if (mReadBuffer != index)
		{
			glReadBuffer(GL_COLOR_ATTACHMENT0 + index);
			mReadBuffer = index;
		}
	}
	void GLFramebuffer::BindDrawBuffers(const std::vector<uint32_t> &indices)
	{
		if (mDrawBuffers != indices)
		{
			std::vector<GLenum> bufs(indices);
			std::transform(indices.begin(), indices.end(), bufs.begin(), [](auto e) {
				return GL_COLOR_ATTACHMENT0 + e;
			});
			glDrawBuffers(static_cast<GLsizei>(bufs.size()), bufs.data());
			mDrawBuffers = indices;
		}
	}
} // namespace Shit
