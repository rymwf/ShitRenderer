/**
 * @file GLImage.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitImage.h>
#include "GLPrerequisites.h"
namespace Shit
{

	class GLImage final : public Image
	{
		GLuint mHandle;
		GLStateManager *mpStateManager;

	public:
		GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo);
		~GLImage() override
		{
			if (static_cast<bool>(mCreateInfo.usageFlags & ImageUsageFlagBits::TRANSIENT_ATTACHMENT_BIT))
			{
				mpStateManager->NotifyReleasedRenderbuffer(mHandle);
				glDeleteRenderbuffers(1, &mHandle);
			}
			else
			{
				mpStateManager->NotifyReleasedTexture(mHandle);
				glDeleteTextures(1, &mHandle);
			}
		}
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}

		/**
		 * @brief if image is renderer buffer or multisample image, do not use this method
		 * 
		 * @param imageSubData 
		 */
		void UpdateImageSubData(const ImageSubData &imageSubData);
	};
}