/**
 * @file ShitSwapchain.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitImage.hpp"

namespace Shit
{
	class Swapchain
	{
	protected:
		SwapchainCreateInfo mCreateInfo;
		QueueFamilyIndex mPresentQueueFamilyIndex{0, 1};

		std::vector<std::unique_ptr<Image>> mImages;

	public:
		Swapchain(const SwapchainCreateInfo &createInfo) : mCreateInfo(createInfo) {}
		constexpr QueueFamilyIndex GetPresentQueueFamilyIndex() const
		{
			return mPresentQueueFamilyIndex;
		}
		virtual ~Swapchain() {}
		constexpr const SwapchainCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}
		void GetImages(std::vector<Image *> &images) const
		{
			images.resize(mImages.size());
			std::transform(mImages.begin(), mImages.end(), images.begin(), [](auto &&e) { return e.get(); });
		}
		Image *GetImageByIndex(uint32_t index) const
		{
			return mImages[index].get();
		}
		uint32_t GetImageNum() const
		{
			return static_cast<uint32_t>(mImages.size());
		}
		virtual Result GetNextImage(const GetNextImageInfo &info, uint32_t& index) = 0;
	};
} // namespace Shit
