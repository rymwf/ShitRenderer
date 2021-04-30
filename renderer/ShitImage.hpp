/**
 * @file ShitImage.hpp
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
		virtual void UpdateSubData(uint32_t mipLevel, const Rect3D &rect, const void *pData) = 0;
		virtual void MapMemory(uint64_t offset, uint64_t size, void **ppData) = 0;
		virtual void UnMapMemory() = 0;
		virtual void FlushMappedMemoryRange(uint64_t offset, uint64_t size) = 0;

		virtual void GenerateMipmaps(Filter filter) = 0;

		//virtual void TransformLayout([[maybe_unused]] uint32_t baseMiplevel, uint32_t levelCount, [[maybe_unused]] ImageLayout dstImageLayout)
		//{
		//}
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
