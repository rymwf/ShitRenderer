/**
 * @file GLSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLSwapchain.h"
#include "GLImage.h"
#include "GLDevice.h"
#include "GLFence.h"
#include "GLSemaphore.h"
namespace Shit
{
	GLSwapchain::GLSwapchain(GLDevice *pDevice, GLStateManager *pStateManager, const SwapchainCreateInfo &createInfo)
		: Swapchain(createInfo), mpDevice(pDevice), mpStateManager(pStateManager)
	{
		std::function<void(const Event &)> func = std::bind(&GLSwapchain::ProcessWindowEvent, this, std::placeholders::_1);
		mProcessWindowEventCallable = std::make_shared<std::function<void(const Event &)>>(func);

#ifdef CLIP_ORIGIN_UPPER_LEFT
		mpStateManager->ClipControl(GL_UPPER_LEFT, GL_ZERO_TO_ONE);
#else
		mpStateManager->ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
#endif

		if (createInfo.format == ShitFormat::RGBA8_SRGB)
			mpStateManager->EnableCapability(GL_FRAMEBUFFER_SRGB);
		else
			mpStateManager->DisableCapability(GL_FRAMEBUFFER_SRGB);

#ifndef NDEBUG
		GL::listGLInfo();
#endif
		mpDevice->SetPresentMode(mCreateInfo.presentMode);

		CreateImages(GetSwapchainImageCount());
		CreateFramebuffer();
	}
	GLSwapchain::~GLSwapchain()
	{
		mpDevice->Destroy(mpFramebuffer);
		for (auto &&e : mpImageViews)
			mpDevice->Destroy(e);
	}
	void GLSwapchain::CreateImages(uint32_t count)
	{
		ImageCreateInfo imageCreateInfo{
			{},
			ImageType::TYPE_2D,
			mCreateInfo.format,
			{mCreateInfo.imageExtent.width,
			 mCreateInfo.imageExtent.height,
			 1u},
			1,
			1,
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		while (count-- > 0)
		{
			mImages.emplace_back(std::make_unique<GLImage>(mpStateManager, imageCreateInfo));
		}
	}
	void GLSwapchain::CreateFramebuffer()
	{
		ImageViewCreateInfo imageViewCreateInfo{nullptr,
												ImageViewType::TYPE_2D,
												mCreateInfo.format,
												{},
												{0, 1, 0, 1}};
		for (auto &&e : mImages)
		{
			imageViewCreateInfo.pImage = e.get();
			mpImageViews.emplace_back(mpDevice->Create(imageViewCreateInfo));
		}
		FramebufferCreateInfo createInfo{nullptr,
										 mpImageViews,
										 createInfo.extent,
										 1};
		mpFramebuffer = mpDevice->Create(createInfo);
	}

	/**
	 * @brief TODO: handle not ready, timeout and other cases
	 * 
	 * @param info 
	 * @param index 
	 * @return Result 
	 */
	Result GLSwapchain::GetNextImage(const GetNextImageInfo &info, uint32_t &index)
	{
		if (mOutDated)
			return Result::SHIT_ERROR_OUT_OF_DATE;
		if (info.pFence)
			static_cast<GLFence *>(info.pFence)->Reset();
		if (info.pSemaphore)
			static_cast<GLSemaphore *>(info.pSemaphore)->Reset();
		index = mAvailableImageIndex %= mImages.size();
		mAvailableImageIndex = index + 1;
		return Result::SUCCESS;
	}

	void GLSwapchain::ProcessWindowEvent(const Event &ev)
	{
		std::visit(
			[this](auto &&arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, WindowResizeEvent>)
				{
					this->mOutDated = true;
				}
			},
			ev.value);
	}
	void GLSwapchain::SwapBuffer() const
	{
		mpDevice->SwapBuffer();
	}
} // namespace Shit
