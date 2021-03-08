/**
 * @file GLImage.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLImage.h"
namespace Shit
{
	GLImage::GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo)
		: mpStateManager(pStateManager), Image(createInfo)
	{
		if (static_cast<bool>(createInfo.usageFlags & ImageUsageFlagBits::TRANSIENT_ATTACHMENT_BIT))
		{
			glGenRenderbuffers(1, &mHandle);
			mpStateManager->PushRenderbuffer(mHandle);
			if (createInfo.samples > SampleCountFlagBits::BIT_1)
			{
				glRenderbufferStorageMultisample(
					GL_RENDERBUFFER,
					static_cast<GLsizei>(createInfo.samples),
					MapInternalFormat(createInfo.format),
					createInfo.extent.width,
					createInfo.extent.height);
			}
			else
			{
				glRenderbufferStorage(
					GL_RENDERBUFFER,
					MapInternalFormat(createInfo.format),
					createInfo.extent.width,
					createInfo.extent.height);
			}
			mpStateManager->PopRenderbuffer();
		}
		else
		{
			glGenTextures(1, &mHandle);
			GLenum target = Map(createInfo.imageType, createInfo.samples);
			mpStateManager->PushTexture(0, target, mHandle);
			//if (static_cast<bool>(createInfo.flags & ImageCreateFlagBits::MUTABLE_FORMAT_BIT))
			if constexpr (0)
			{
				if (createInfo.samples > SampleCountFlagBits::BIT_1)
				{
					glTexImage3DMultisample(
						target,
						static_cast<GLsizei>(createInfo.samples),
						MapInternalFormat(createInfo.format),
						createInfo.extent.width,
						createInfo.extent.height,
						createInfo.extent.depth,
						GL_FALSE);
				}
				else
				{
					switch (createInfo.imageType)
					{
					case ImageType::TYPE_1D:
						glTexImage2D(
							target,
							0,
							MapInternalFormat(createInfo.format),
							createInfo.extent.width,
							createInfo.extent.height,
							0,
							GL_RGBA,
							GL_UNSIGNED_BYTE,
							nullptr);
						break;
					case ImageType::TYPE_2D:
					case ImageType::TYPE_3D:
						glTexImage3D(
							target,
							0,
							MapInternalFormat(createInfo.format),
							createInfo.extent.width,
							createInfo.extent.height,
							createInfo.extent.depth,
							0,
							GL_RGBA,
							GL_UNSIGNED_BYTE,
							nullptr);
						break;
					}
				}
			}
			else
			{
				if (createInfo.samples > SampleCountFlagBits::BIT_1)
				{
					glTexStorage3DMultisample(
						target,
						static_cast<GLsizei>(createInfo.samples),
						MapInternalFormat(createInfo.format),
						createInfo.extent.width,
						createInfo.extent.height,
						createInfo.extent.depth,
						GL_FALSE);
				}
				else
				{
					switch (createInfo.imageType)
					{
					case ImageType::TYPE_1D:
						glTexStorage2D(
							target,
							createInfo.mipLevels,
							MapInternalFormat(createInfo.format),
							createInfo.extent.width,
							createInfo.extent.height);
						break;
					case ImageType::TYPE_2D:
					case ImageType::TYPE_3D:
						glTexStorage3D(
							target,
							createInfo.mipLevels,
							MapInternalFormat(createInfo.format),
							createInfo.extent.width,
							createInfo.extent.height,
							createInfo.extent.depth);
						break;
					}
				}
			}
			mpStateManager->PopTexture();
		}
	}
	void GLImage::UpdateImageSubData(const ImageSubData &imageSubData)
	{
		GLenum target = Map(mCreateInfo.imageType, mCreateInfo.samples);
		mpStateManager->PushTexture(0, target, mHandle);
		switch (mCreateInfo.imageType)
		{
		case ImageType::TYPE_1D:
			glTexSubImage2D(
				target,
				imageSubData.mipLevel,
				imageSubData.rect.offset.x,
				imageSubData.rect.offset.y,
				imageSubData.rect.extent.width,
				imageSubData.rect.extent.height,
				MapExternalFormat(imageSubData.format),
				Map(imageSubData.dataType),
				imageSubData.data);
			break;
		case ImageType::TYPE_2D:
		case ImageType::TYPE_3D:
			glTexSubImage3D(
				target,
				imageSubData.mipLevel,
				imageSubData.rect.offset.x,
				imageSubData.rect.offset.y,
				imageSubData.rect.offset.z,
				imageSubData.rect.extent.width,
				imageSubData.rect.extent.height,
				imageSubData.rect.extent.depth,
				MapExternalFormat(imageSubData.format),
				Map(imageSubData.dataType),
				imageSubData.data);
			break;
		}
		mpStateManager->PopTexture();
	}
} // namespace Shit
