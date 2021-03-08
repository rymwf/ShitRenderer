/**
 * @file ShitSwapchain.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitImage.h"

namespace Shit
{
	class Swapchain
	{
	protected:
		SwapchainCreateInfo mCreateInfo;
		QueueFamilyIndex mPresentQueueFamilyIndex{0, INT_MAX};

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
			for (auto &&e : mImages)
				images.emplace_back(e.get());
		}
	};
} // namespace Shit
