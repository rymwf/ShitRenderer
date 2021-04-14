/**
 * @file GLImage.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLImage.hpp"
#include "GLSampler.hpp"
namespace Shit
{
	GLImage::~GLImage()
	{
		if (mIsRenderbuffer)
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
	GLImage::GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, const void *pData)
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
			mpStateManager->PushTextureUnit(0, target, mHandle);
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
			if (pData)
			{
				UpdateSubData(0, {{}, mCreateInfo.extent}, pData);
				//TODO: generate mipmap using different filter
				if (mCreateInfo.generateMipmap)
					glGenerateMipmap(target);
				mpStateManager->PopTextureUnit();
			}
		}
	}
	void GLImage::UpdateSubData(uint32_t mipLevel, const Rect3D &rect, const void *pData)
	{
		GLenum target = Map(mCreateInfo.imageType, mCreateInfo.samples);
		mpStateManager->PushTextureUnit(0, target, mHandle);
		switch (mCreateInfo.imageType)
		{
		case ImageType::TYPE_1D:
			glTexSubImage2D(
				target,
				mipLevel,
				rect.offset.x,
				rect.offset.y,
				rect.extent.width,
				rect.extent.height,
				MapExternalFormat(mCreateInfo.format),
				MapDataTypeFromFormat(mCreateInfo.format),
				pData);
			break;
		case ImageType::TYPE_2D:
		case ImageType::TYPE_3D:
			glTexSubImage3D(
				target,
				mipLevel,
				rect.offset.x,
				rect.offset.y,
				rect.offset.z,
				rect.extent.width,
				rect.extent.height,
				rect.extent.depth,
				MapExternalFormat(mCreateInfo.format),
				MapDataTypeFromFormat(mCreateInfo.format),
				pData);
			break;
		}
		mpStateManager->PopTextureUnit();
	}
	void GLImage::MapMemory([[maybe_unused]] uint64_t offset, [[maybe_unused]] uint64_t size, void **ppData)
	{
		mpStateManager->BindBuffer(GL_PIXEL_PACK_BUFFER, 0);
		auto target = Map(mCreateInfo.imageType, mCreateInfo.samples);
		mpStateManager->BindTextureUnit(0, target, mHandle);
		mTempData.resize(mCreateInfo.extent.width * mCreateInfo.extent.height * GetFormatComponentNum(mCreateInfo.format));
		glGetTexImage(target, 0, MapExternalFormat(mCreateInfo.format), MapDataTypeFromFormat(mCreateInfo.format), mTempData.data());
		*ppData = mTempData.data() + offset;
	}
	void GLImage::UnMapMemory()
	{
		UpdateSubData(0, Rect3D{{}, mCreateInfo.extent}, mTempData.data());
	}

	//===================================================================

	GLImageView::GLImageView(GLStateManager *pStateManger, const ImageViewCreateInfo &createInfo)
		: ImageView(createInfo), mpStateManger(pStateManger)
	{
		auto pImage = static_cast<GLImage *>(createInfo.pImage);
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
		mpStateManger->PushTextureUnit(0, target, mHandle);
		if (externalFormat == GL_DEPTH_STENCIL || externalFormat == GL_DEPTH_COMPONENT)
		{
			glTexParameteri(target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_DEPTH_COMPONENT);
		}
		else if (externalFormat == GL_STENCIL_INDEX)
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
		mpStateManger->PopTextureUnit();
	}
	void GLImageView::SetSampler(Sampler *pSampler)
	{
		if (mpSampler == pSampler)
			return;
		mpSampler = pSampler;
		GLenum target = Map(mCreateInfo.viewType, mCreateInfo.pImage->GetCreateInfoPtr()->samples);
		mpStateManger->PushTextureUnit(0, target, mHandle);

		auto pSamplerCreateInfo = pSampler->GetCreateInfoPtr();

		glTexParameteri(target, GL_TEXTURE_MAG_FILTER, Map(pSamplerCreateInfo->magFilter));
		if (pSamplerCreateInfo->mipmapMode == SamplerMipmapMode::LINEAR)
		{
			if (pSamplerCreateInfo->minFilter == Filter::LINEAR)
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			else
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		}
		else
		{
			if (pSamplerCreateInfo->minFilter == Filter::LINEAR)
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			else
				glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		}
		glTexParameterf(target, GL_TEXTURE_LOD_BIAS, pSamplerCreateInfo->mipLodBias);
		glTexParameterf(target, GL_TEXTURE_MIN_LOD, pSamplerCreateInfo->minLod);
		glTexParameterf(target, GL_TEXTURE_MAX_LOD, pSamplerCreateInfo->maxLod);
		glTexParameteri(target, GL_TEXTURE_WRAP_S, Map(pSamplerCreateInfo->wrapModeU));
		glTexParameteri(target, GL_TEXTURE_WRAP_T, Map(pSamplerCreateInfo->wrapModeV));
		glTexParameteri(target, GL_TEXTURE_WRAP_R, Map(pSamplerCreateInfo->wrapModeW));
		if (pSamplerCreateInfo->compareEnable)
		{
			glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
			glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, Map(pSamplerCreateInfo->compareOp));
		}

		auto borderColor = Map(pSamplerCreateInfo->borderColor);
		std::visit([target](auto &&arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::array<float, 4>>)
				glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, arg.data());
			else if constexpr (std::is_same_v<T, std::array<int32_t, 4>>)
				glTexParameterIiv(target, GL_TEXTURE_BORDER_COLOR, arg.data());
		},
				   borderColor);
		mpStateManger->PopTextureUnit();
	}

	GLImageView::~GLImageView()
	{
		if (!static_cast<GLImage *>(mCreateInfo.pImage)->IsRenderbuffer())
			glDeleteTextures(1, &mHandle);
	}
} // namespace Shit
