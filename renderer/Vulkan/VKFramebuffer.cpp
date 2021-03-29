/**
 * @file VKFramebuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKFramebuffer.hpp"
#include "VKRenderPass.hpp"
#include "VKImage.hpp"
namespace Shit
{

	VKFramebuffer::VKFramebuffer(VkDevice device, const FramebufferCreateInfo &createInfo)
		: Framebuffer(createInfo), mDevice(device)
	{
		std::vector<VkImageView> attachments;
		for (auto &&e : createInfo.attachments)
		{
			attachments.emplace_back(static_cast<VKImageView *>(e)->GetHandle());
		}
		VkFramebufferCreateInfo info{
			VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			nullptr,
			0,
			static_cast<VKRenderPass *>(createInfo.pRenderPass)->GetHandle(),
			static_cast<uint32_t>(attachments.size()),
			attachments.data(),
			createInfo.extent.width,
			createInfo.extent.height,
			createInfo.layers};

		if (vkCreateFramebuffer(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create framebuffer");
	}
} // namespace Shit
