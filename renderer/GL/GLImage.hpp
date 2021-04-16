/**
 * @file GLImage.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitImage.hpp>
#include "GLPrerequisites.hpp"
namespace Shit
{
	class GLImage final : public Image
	{
		GLuint mHandle;
		GLStateManager *mpStateManager;
		bool mIsRenderbuffer{};
		std::vector<unsigned char> mTempData;

	public:
		GLImage(GLStateManager *pStateManager, const ImageCreateInfo &createInfo, const void *pData);
		~GLImage() override;
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}

		/**
		 * @brief if image is renderer buffer or multisample image, do not use this method
		 * 
		 * @param imageSubData 
		 */
		void UpdateSubData(uint32_t mipLevel, const Rect3D &rect, const void *pData) override;
		constexpr bool IsRenderbuffer() const
		{
			return mIsRenderbuffer;
		}
		void MapMemory(uint64_t offset, uint64_t size, void **ppData) override;
		void UnMapMemory() override;
		void FlushMappedMemoryRange(uint64_t offset, uint64_t size) override;
	};

	class GLImageView final : public ImageView
	{
		GLuint mHandle;
		GLStateManager *mpStateManger;
		Sampler *mpSampler;

	public:
		GLImageView(GLStateManager *pStateManger, const ImageViewCreateInfo &createInfo);
		~GLImageView() override;
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
		void SetSampler(Sampler *pSampler);
	};
}