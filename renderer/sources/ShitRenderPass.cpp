/**
 * @file ShitRenderPass.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "ShitRenderPass.hpp"

namespace Shit {
RenderPass::RenderPass(const RenderPassCreateInfo &createInfo) {
  mCreateInfo.attachmentCount = createInfo.attachmentCount;
  mCreateInfo.subpassCount = createInfo.subpassCount;
  mCreateInfo.dependencyCount = createInfo.dependencyCount;
  if (mCreateInfo.attachmentCount) {
    mCreateInfo.pAttachments =
        new AttachmentDescription[mCreateInfo.attachmentCount];
    memcpy((void *)mCreateInfo.pAttachments, createInfo.pAttachments,
           sizeof(AttachmentDescription) * mCreateInfo.attachmentCount);
  }
  if (mCreateInfo.dependencyCount) {
    mCreateInfo.pSubpassDependencies =
        new SubpassDependency[mCreateInfo.dependencyCount];
    memcpy((void *)mCreateInfo.pSubpassDependencies,
           createInfo.pSubpassDependencies,
           sizeof(SubpassDependency) * mCreateInfo.dependencyCount);
  }

  auto ppSubpasses = new SubpassDescription[mCreateInfo.subpassCount];
  memset(ppSubpasses, 0, sizeof(SubpassDescription) * mCreateInfo.subpassCount);
  uint32_t tempCount;
  for (uint32_t i = 0; i < mCreateInfo.subpassCount; ++i) {
    auto src_pSubpasses = createInfo.pSubpasses[i];
    ppSubpasses[i].pipelineBindPoint = src_pSubpasses.pipelineBindPoint;

    // input pAttachments
    if ((tempCount = ppSubpasses[i].inputAttachmentCount =
             src_pSubpasses.inputAttachmentCount) != 0) {
      ppSubpasses[i].pInputAttachments = new AttachmentReference[tempCount];
      memcpy((void *)ppSubpasses[i].pInputAttachments,
             src_pSubpasses.pInputAttachments,
             sizeof(AttachmentReference) * tempCount);
    }

    // color attachments
    if ((tempCount = ppSubpasses[i].colorAttachmentCount =
             src_pSubpasses.colorAttachmentCount) != 0) {
      ppSubpasses[i].pColorAttachments = new AttachmentReference[tempCount];
      memcpy((void *)ppSubpasses[i].pColorAttachments,
             src_pSubpasses.pColorAttachments,
             sizeof(AttachmentReference) * tempCount);
    }

    // resolve attachments
    if ((src_pSubpasses.pResolveAttachments) != 0) {
      ppSubpasses[i].pResolveAttachments = new AttachmentReference[tempCount];
      memcpy((void *)ppSubpasses[i].pResolveAttachments,
             src_pSubpasses.pResolveAttachments,
             sizeof(AttachmentReference) * tempCount);
    }
    // depth attachments
    if (src_pSubpasses.pDepthStencilAttachment)
      ppSubpasses[i].pDepthStencilAttachment =
          new AttachmentReference(*src_pSubpasses.pDepthStencilAttachment);
  }
  if (createInfo.pMultiviewCreateInfo) {
    auto a = new RenderPassMultiViewCreateInfo{};

    if ((a->subpassCount = createInfo.pMultiviewCreateInfo->subpassCount) !=
        0) {
      a->pViewMasks = new uint32_t[a->subpassCount];
      memcpy((void *)a->pViewMasks, createInfo.pMultiviewCreateInfo->pViewMasks,
             sizeof(uint32_t) * a->subpassCount);
    };
    if ((a->dependencyCount =
             createInfo.pMultiviewCreateInfo->dependencyCount) != 0) {
      a->pViewOffsets = new int32_t[a->dependencyCount];
      memcpy((void *)a->pViewOffsets,
             createInfo.pMultiviewCreateInfo->pViewOffsets,
             sizeof(uint32_t) * a->dependencyCount);
    }
    if ((a->correlationMaskCount =
             createInfo.pMultiviewCreateInfo->correlationMaskCount) != 0) {
      a->pCorrelationMasks = new uint32_t[a->correlationMaskCount];
      memcpy((void *)a->pCorrelationMasks,
             createInfo.pMultiviewCreateInfo->pCorrelationMasks,
             sizeof(uint32_t) * a->correlationMaskCount);
    }
    mCreateInfo.pMultiviewCreateInfo = a;
  }

  mCreateInfo.pSubpasses = ppSubpasses;
}

RenderPass::~RenderPass() {
  delete[] mCreateInfo.pAttachments;
  delete[] mCreateInfo.pSubpassDependencies;
  for (uint32_t i = 0; i < mCreateInfo.subpassCount; ++i) {
    delete[] mCreateInfo.pSubpasses[i].pInputAttachments;
    delete[] mCreateInfo.pSubpasses[i].pColorAttachments;
    delete[] mCreateInfo.pSubpasses[i].pResolveAttachments;
    delete mCreateInfo.pSubpasses[i].pDepthStencilAttachment;
  }
  delete[] mCreateInfo.pSubpasses;

  if (mCreateInfo.pMultiviewCreateInfo) {
    delete[] mCreateInfo.pMultiviewCreateInfo->pViewMasks;
    delete[] mCreateInfo.pMultiviewCreateInfo->pViewOffsets;
    delete[] mCreateInfo.pMultiviewCreateInfo->pCorrelationMasks;
    delete mCreateInfo.pMultiviewCreateInfo;
  }
}
}  // namespace Shit
