/**
 * @file ShitImage.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
namespace Shit
{
	class Image
	{
	protected:
		bool mIsSwapchainImage{};
		ImageCreateInfo mCreateInfo;
		Image(const ImageCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		Image() = default;

	public:
		virtual ~Image() {}
		constexpr const ImageCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
		virtual void UpdateSubData(uint32_t mipLevel, const Rect3D rect, const void *pData) = 0;
	};

	class ImageView
	{
	protected:
		ImageViewCreateInfo mCreateInfo;
		ImageView(const ImageViewCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~ImageView() {}
		constexpr const ImageViewCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
	};

}
