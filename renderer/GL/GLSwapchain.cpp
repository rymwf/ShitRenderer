/**
 * @file GLSwapchain.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLSwapchain.hpp"
#include "GLImage.hpp"
#include "GLDevice.hpp"
#include "GLFence.hpp"
#include "GLSemaphore.hpp"
#include "GLRenderPass.hpp"
#include "GLFramebuffer.hpp"
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
		
		//cubemap seamless
		mpStateManager->EnableCapability(GL_TEXTURE_CUBE_MAP_SEAMLESS);

#ifndef NDEBUG
		GL::listGLInfo();
#endif
		mpDevice->SetPresentMode(mCreateInfo.presentMode);

		CreateImages(GetSwapchainImageCount());
		CreateRenderPass();
		CreateFramebuffer();
	}
	GLSwapchain::~GLSwapchain()
	{
		mpDevice->Destroy(mpRenderPass);
		mpDevice->Destroy(mpFramebuffer);
		for (auto &&e : mpImageViews)
			mpDevice->Destroy(e);
	}
	void GLSwapchain::CreateRenderPass()
	{
		std::vector<AttachmentDescription> attachmentDescs(
			mImages.size(),
			{mCreateInfo.format,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,
			 AttachmentStoreOp::DONT_CARE,
			 AttachmentLoadOp::DONT_CARE,
			 AttachmentStoreOp::DONT_CARE,
			 ImageLayout::UNDEFINED, //inital layout
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL});
		uint32_t len = static_cast<uint32_t>(attachmentDescs.size());
		std::vector<AttachmentReference> attachments(len);
		for (uint32_t i = 0; i < len; ++i)
			attachments[i] = {i};
		SubpassDescription subpassDesc{
			PipelineBindPoint::GRAPHICS,
			{},
			std::move(attachments),
		};
		RenderPassCreateInfo createInfo{
			std::move(attachmentDescs),
			{std::move(subpassDesc)}};
		mpRenderPass = mpDevice->Create(createInfo);
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
			mImages.emplace_back(std::make_unique<GLImage>(mpStateManager, imageCreateInfo, nullptr));
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

		FramebufferCreateInfo createInfo{mpRenderPass, //renderpass
										 mpImageViews,
										 createInfo.extent,
										 1};
		mpFramebuffer = mpDevice->Create(createInfo);
		static_cast<GLFramebuffer *>(mpFramebuffer)->SetRenderFBOAttachment(0);
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
