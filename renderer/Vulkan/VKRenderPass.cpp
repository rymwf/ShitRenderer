/**
 * @file VKRenderPass.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKRenderPass.hpp"

namespace Shit
{

	VKRenderPass::VKRenderPass(VkDevice device, const RenderPassCreateInfo &createInfo)
		: RenderPass(createInfo), mDevice(device)
	{
		std::vector<VkAttachmentDescription> attachmentDescription;
		for (auto &&e : createInfo.attachments)
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

		std::vector<VkSubpassDescription> subPasses(createInfo.subpasses.size());
		std::vector<VkAttachmentReference> inputAttachments;
		std::vector<VkAttachmentReference> colorAttachments;
		std::vector<VkAttachmentReference> resolveAttachments;
		VkAttachmentReference depthStencilAttachment{};

		std::transform(createInfo.subpasses.begin(), createInfo.subpasses.end(), subPasses.begin(), [&](auto &&e) {
			inputAttachments.resize(e.inputAttachments.size());
			colorAttachments.resize(e.colorAttachments.size());
			resolveAttachments.resize(e.resolveAttachments.size());
			std::transform(std::execution::par, e.inputAttachments.begin(), e.inputAttachments.end(), inputAttachments.begin(), [](auto &&a) {
				return VkAttachmentReference{a.attachment, Map(a.layout)};
			});
			std::transform(std::execution::par, e.colorAttachments.begin(), e.colorAttachments.end(), colorAttachments.begin(), [](auto &&a) {
				return VkAttachmentReference{a.attachment, Map(a.layout)};
			});
			std::transform(std::execution::par, e.resolveAttachments.begin(), e.resolveAttachments.end(), resolveAttachments.begin(), [](auto &&a) {
				return VkAttachmentReference{a.attachment, Map(a.layout)};
			});
			if (e.depthStencilAttachment.has_value())
				depthStencilAttachment = {e.depthStencilAttachment->attachment, Map(e.depthStencilAttachment->layout)};
			return VkSubpassDescription{
				0,
				Map(e.pipelineBindPoint),
				static_cast<uint32_t>(inputAttachments.size()),
				inputAttachments.data(),
				static_cast<uint32_t>(colorAttachments.size()),
				colorAttachments.data(),
				resolveAttachments.data(), //resolve attachment, used for multisampling color attachment
				e.depthStencilAttachment.has_value() ? &depthStencilAttachment : nullptr,
				0, nullptr //reserve attachments
			};
		});

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