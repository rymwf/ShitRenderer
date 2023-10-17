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

namespace Shit {

VKRenderPass::VKRenderPass(VKDevice *device, const RenderPassCreateInfo &createInfo)
    : RenderPass(createInfo), VKDeviceObject(device) {
    std::vector<VkAttachmentDescription> attachmentDescription;
    for (uint32_t i = 0; i < mCreateInfo.attachmentCount; ++i) {
        auto &&e = mCreateInfo.pAttachments[i];
        attachmentDescription.emplace_back(VkAttachmentDescription{
            0, Map(e.format), static_cast<VkSampleCountFlagBits>(e.samples), Map(e.loadOp), Map(e.storeOp),
            Map(e.stencilLoadOp), Map(e.stencilStoreOp), Map(e.initialLayout), Map(e.finalLayout)});
    }

    std::vector<VkSubpassDescription> subPasses(mCreateInfo.subpassCount);
    std::vector<VkAttachmentReference> attachments;
    attachments.reserve(mCreateInfo.attachmentCount * mCreateInfo.subpassCount);
    // std::vector<VkAttachmentReference> inputAttachments;
    // std::vector<VkAttachmentReference> colorAttachments;
    // std::vector<VkAttachmentReference> resolveAttachments;
    // std::vector<VkAttachmentReference> depthStencilAttachments{};

    for (uint32_t i = 0, j; i < mCreateInfo.subpassCount; ++i) {
        auto &&e = mCreateInfo.pSubpasses[i];
        subPasses[i] = {
            0, Map(e.pipelineBindPoint), e.inputAttachmentCount, 0, e.colorAttachmentCount,
        };
        if (e.inputAttachmentCount) subPasses[i].pInputAttachments = attachments.data() + attachments.size();
        for (j = 0; j < e.inputAttachmentCount; ++j)
            attachments.emplace_back(e.pInputAttachments[j].attachment, Map(e.pInputAttachments[j].layout));
        if (e.colorAttachmentCount) subPasses[i].pColorAttachments = attachments.data() + attachments.size();
        for (j = 0; j < e.colorAttachmentCount; ++j)
            attachments.emplace_back(e.pColorAttachments[j].attachment, Map(e.pColorAttachments[j].layout));

        if (e.pResolveAttachments) {
            subPasses[i].pResolveAttachments = attachments.data() + attachments.size();
            for (j = 0; j < e.colorAttachmentCount; ++j)
                attachments.emplace_back(e.pResolveAttachments[j].attachment, Map(e.pResolveAttachments[j].layout));
        }
        if (e.pDepthStencilAttachment) {
            subPasses[i].pDepthStencilAttachment = attachments.data() + attachments.size();
            attachments.emplace_back(e.pDepthStencilAttachment->attachment, Map(e.pDepthStencilAttachment->layout));
        }
    }
    std::vector<VkSubpassDependency> subPassDependencies(mCreateInfo.dependencyCount);
    for (uint32_t i = 0; i < mCreateInfo.dependencyCount; ++i) {
        auto &&e = mCreateInfo.pSubpassDependencies[i];
        subPassDependencies[i] = {e.srcSubpass,
                                  e.dstSubpass,
                                  Map(e.srcStageMask),
                                  Map(e.dstStageMask),
                                  Map(e.srcAccessFlagBits),
                                  Map(e.dstAccessFlagBits)};
    }
    void *pNext = 0;
    VkRenderPassMultiviewCreateInfo multiviewCI;
    if (mCreateInfo.pMultiviewCreateInfo) {
        multiviewCI = {VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO,
                       0,
                       mCreateInfo.pMultiviewCreateInfo->subpassCount,
                       mCreateInfo.pMultiviewCreateInfo->pViewMasks,
                       mCreateInfo.pMultiviewCreateInfo->dependencyCount,
                       mCreateInfo.pMultiviewCreateInfo->pViewOffsets,
                       mCreateInfo.pMultiviewCreateInfo->correlationMaskCount,
                       mCreateInfo.pMultiviewCreateInfo->pCorrelationMasks};
        pNext = &multiviewCI;
    }

    VkRenderPassCreateInfo info{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
                                pNext,
                                0,
                                static_cast<uint32_t>(attachmentDescription.size()),
                                attachmentDescription.data(),
                                static_cast<uint32_t>(subPasses.size()),
                                subPasses.data(),
                                static_cast<uint32_t>(subPassDependencies.size()),
                                subPassDependencies.data()};

    CHECK_VK_RESULT(vkCreateRenderPass(mpDevice->GetHandle(), &info, nullptr, &mHandle))
}
}  // namespace Shit