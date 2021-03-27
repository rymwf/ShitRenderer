/**
 * @file VKRenderPass.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKRenderPass.h"

namespace Shit
{

	VKRenderPass::VKRenderPass(VkDevice device, const RenderPassCreateInfo &createInfo)
		: RenderPass(createInfo), mDevice(device)
	{
		std::vector<VkAttachmentDescription> attachmentDescription;
		for(auto&& e:createInfo.attachments)
		{
			attachmentDescription.emplace_back(
				VkAttachmentDescription{
					0,
					Map(e.format),
					static_cast<VkSampleCountFlagBits>(e.samples),
					Map(e.loadOp),
					Map(e.storeOp),
					Map(e.stencilLoadOp),
					Map(e.stencilStoreOp),
					Map(e.initialLayout),
					Map(e.finalLayout)});
		}


		std::vector<VkSubpassDescription> subPasses;
		std::vector<VkAttachmentReference> colorAttachments;
		VkAttachmentReference resolveAttachment;
		VkAttachmentReference depthStencilAttachment;
		for(auto&& e:createInfo.subpasses)
		{
			for (auto &&a : e.colorAttachments)
			{
				colorAttachments.emplace_back(
					VkAttachmentReference{
						a.attachment,
						Map(a.layout)});
			}
			if(e.resolveAttachment.has_value())
			{
				resolveAttachment.attachment = e.resolveAttachment->attachment;
				resolveAttachment.layout = Map(e.resolveAttachment->layout);
			}
			if(e.depthStencilAttachment.has_value())
			{
				depthStencilAttachment.attachment = e.depthStencilAttachment->attachment;
				depthStencilAttachment.layout = Map(e.depthStencilAttachment->layout);
			}
			subPasses.emplace_back(
				VkSubpassDescription{
					0,
					Map(e.pipelineBindPoint),
					0,
					nullptr,
					static_cast<uint32_t>(colorAttachments.size()),
					colorAttachments.data(),
					e.resolveAttachment.has_value() ? &resolveAttachment : nullptr, //resolve attachment, used for multisampling color attachment &depthStencilAttachment,
					e.depthStencilAttachment.has_value() ? &depthStencilAttachment : nullptr,
					0,
					nullptr});
		}

		std::vector<VkSubpassDependency> subPassDependencies{
			{
				VK_SUBPASS_EXTERNAL,
				0,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			}};

		VkRenderPassCreateInfo info{
			VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(attachmentDescription.size()),
			attachmentDescription.data(),
			static_cast<uint32_t>(subPasses.size()),
			subPasses.data(),
			static_cast<uint32_t>(subPassDependencies.size()),
			subPassDependencies.data()};

		if (vkCreateRenderPass(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create renderpass");
	}
}