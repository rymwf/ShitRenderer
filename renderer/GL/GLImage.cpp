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
		: Image(createInfo), mpStateManager(pStateManager)
	{
		if (!static_cast<bool>(createInfo.flags & ImageCreateFlagBits::MUTABLE_FORMAT_BIT) &&
			static_cast<bool>(createInfo.usageFlags & (ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT)) &&
			!static_cast<bool>(createInfo.usageFlags & ImageUsageFlagBits::INPUT_ATTACHMENT_BIT) &&
			createInfo.mipLevels == 1 &&
			createInfo.extent.depth == 1 &&
			createInfo.arrayLayers == 1)
		{
			mIsRenderbuffer = true;
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

	//===================================================================

	GLImageView::GLImageView(GLStateManager *pStateManger, const ImageViewCreateInfo &createInfo)
		: ImageView(createInfo), mpStateManger(pStateManger)
	{
		auto pImage=static_cast<GLImage *>(createInfo.pImage);
		if (pImage->IsRenderbuffer())
		{
			mHandle = pImage->GetHandle();
			return;
		}
		auto target = Map(createInfo.viewType, pImage->GetCreateInfoPtr()->samples);
		auto externalFormat = MapInternalFormat(createInfo.format);
		glGenTextures(1, &mHandle);
		if (GLEW_VERSION_4_3)
		{
			glTextureView(mHandle,
						  target,
						  pImage->GetHandle(),
						  externalFormat,
						  createInfo.subresourceRange.baseMipLevel,
						  createInfo.subresourceRange.levelCount,
						  createInfo.subresourceRange.baseArrayLayer,
						  createInfo.subresourceRange.layerCount);
		}
		else if (glIsExtensionSupported("GL_EXT_texture_view"))
		{
			glTextureViewEXT(mHandle,
							 target,
							 pImage->GetHandle(),
							 externalFormat,
							 createInfo.subresourceRange.baseMipLevel,
							 createInfo.subresourceRange.levelCount,
							 createInfo.subresourceRange.baseArrayLayer,
							 createInfo.subresourceRange.layerCount);
		}
		else
		{
			THROW("texture view not supported");
		}
		mpStateManger->PushTexture(0, target, mHandle);
		if (externalFormat == GL_DEPTH_STENCIL || externalFormat == GL_DEPTH_COMPONENT)
		{
			glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
		}
		else if(externalFormat==GL_STENCIL_INDEX)
		{
			glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
		}

		GLint swizzles[4]{
			GL_RED,
			GL_GREEN,
			GL_BLUE,
			GL_ALPHA,
		};
		if (createInfo.components.r != ComponentSwizzle::IDENTITY)
			swizzles[0] = Map(createInfo.components.r);
		if (createInfo.components.g != ComponentSwizzle::IDENTITY)
			swizzles[1] = Map(createInfo.components.g);
		if (createInfo.components.b != ComponentSwizzle::IDENTITY)
			swizzles[2] = Map(createInfo.components.b);
		if (createInfo.components.a != ComponentSwizzle::IDENTITY)
			swizzles[3] = Map(createInfo.components.a);
		glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzles);
		mpStateManger->PopTexture();
	}

} // namespace Shit
