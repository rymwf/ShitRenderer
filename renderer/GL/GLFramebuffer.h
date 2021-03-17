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
#include <renderer/ShitFramebuffer.h>
#include "GLPrerequisites.h"
namespace Shit
{
	class GLFramebuffer final : public Framebuffer
	{
		GLuint mHandle;
		GLStateManager *mpStateManager;

		uint32_t mReadBuffer;
		std::vector<uint32_t> mDrawBuffers;

	public:
		GLFramebuffer(GLStateManager *pStateManager, const FramebufferCreateInfo &createInfo);
		~GLFramebuffer()
		{
			mpStateManager->NotifyReleasedFramebuffer(mHandle);
			glDeleteFramebuffers(1, &mHandle);
		}
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
		void BindReadBuffer(uint32_t index);
		void BindDrawBuffers(const std::vector<uint32_t> &indices);
	};
} // namespace Shit
