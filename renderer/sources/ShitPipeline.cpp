/**
 * @file ShitPipeline.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2022
 *
 */
#include "ShitPipeline.hpp"

namespace Shit {

GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineCreateInfo &createInfo)
    : mCreateInfo(createInfo) {
  mCreateInfo.stages =
      new PipelineShaderStageCreateInfo[mCreateInfo.stageCount];
  memcpy((void *)mCreateInfo.stages, createInfo.stages,
         sizeof(PipelineShaderStageCreateInfo) * mCreateInfo.stageCount);

  for (uint32_t i = 0; i < mCreateInfo.stageCount; ++i) {
    auto &&e = const_cast<SpecializationInfo *>(
        &mCreateInfo.stages[i].specializationInfo);
    auto &&s = createInfo.stages[i].specializationInfo;
    if (e->constantValueCount) {
      e->constantIds = new uint32_t[e->constantValueCount];
      memcpy((void *)e->constantIds, s.constantIds,
             sizeof(uint32_t) * e->constantValueCount);
      e->constantValues = new uint32_t[e->constantValueCount];
      memcpy((void *)e->constantValues, s.constantValues,
             sizeof(uint32_t) * e->constantValueCount);
    }
  }

  if (mCreateInfo.vertexInputState.vertexBindingDescriptionCount) {
    mCreateInfo.vertexInputState.vertexBindingDescriptions =
        new VertexBindingDescription[mCreateInfo.vertexInputState
                                         .vertexBindingDescriptionCount];
    memcpy((void *)mCreateInfo.vertexInputState.vertexBindingDescriptions,
           createInfo.vertexInputState.vertexBindingDescriptions,
           sizeof(VertexBindingDescription) *
               mCreateInfo.vertexInputState.vertexBindingDescriptionCount);
  }
  if (mCreateInfo.vertexInputState.vertexAttributeDescriptionCount) {
    mCreateInfo.vertexInputState.vertexAttributeDescriptions =
        new VertexAttributeDescription[mCreateInfo.vertexInputState
                                           .vertexAttributeDescriptionCount];
    memcpy((void *)mCreateInfo.vertexInputState.vertexAttributeDescriptions,
           createInfo.vertexInputState.vertexAttributeDescriptions,
           sizeof(VertexAttributeDescription) *
               mCreateInfo.vertexInputState.vertexAttributeDescriptionCount);
  }

  // viewport
  if (mCreateInfo.viewportState.viewportCount) {
    mCreateInfo.viewportState.viewports =
        new Viewport[mCreateInfo.viewportState.viewportCount];
    memcpy((void *)mCreateInfo.viewportState.viewports,
           createInfo.viewportState.viewports,
           sizeof(Viewport) * mCreateInfo.viewportState.viewportCount);
  }

  if (mCreateInfo.viewportState.scissorCount) {
    mCreateInfo.viewportState.scissors =
        new Rect2D[mCreateInfo.viewportState.scissorCount];
    memcpy((void *)mCreateInfo.viewportState.scissors,
           createInfo.viewportState.scissors,
           sizeof(Rect2D) * mCreateInfo.viewportState.scissorCount);
  }

  if (mCreateInfo.colorBlendState.attachmentCount) {
    // color blend
    mCreateInfo.colorBlendState.attachments =
        new PipelineColorBlendAttachmentState[mCreateInfo.colorBlendState
                                                  .attachmentCount];
    memcpy((void *)mCreateInfo.colorBlendState.attachments,
           createInfo.colorBlendState.attachments,
           sizeof(PipelineColorBlendAttachmentState) *
               mCreateInfo.colorBlendState.attachmentCount);
  }

  if (mCreateInfo.dynamicState.dynamicStateCount) {
    // dynamic state
    mCreateInfo.dynamicState.dynamicStates =
        new DynamicState[mCreateInfo.dynamicState.dynamicStateCount];
    memcpy((void *)mCreateInfo.dynamicState.dynamicStates,
           createInfo.dynamicState.dynamicStates,
           sizeof(DynamicState) * mCreateInfo.dynamicState.dynamicStateCount);
  }
}

GraphicsPipeline::~GraphicsPipeline() {
  for (uint32_t i = 0; i < mCreateInfo.stageCount; ++i) {
    if (mCreateInfo.stages[i].specializationInfo.constantValueCount) {
      delete[] mCreateInfo.stages[i].specializationInfo.constantIds;
      delete[] mCreateInfo.stages[i].specializationInfo.constantValues;
    }
  }
  if (mCreateInfo.stageCount) delete[] mCreateInfo.stages;
  if (mCreateInfo.vertexInputState.vertexBindingDescriptionCount)
    delete[] mCreateInfo.vertexInputState.vertexBindingDescriptions;
  if (mCreateInfo.vertexInputState.vertexAttributeDescriptionCount)
    delete[] mCreateInfo.vertexInputState.vertexAttributeDescriptions;
  if (mCreateInfo.viewportState.viewportCount)
    delete[] mCreateInfo.viewportState.viewports;
  if (mCreateInfo.viewportState.scissorCount)
    delete[] mCreateInfo.viewportState.scissors;
  if (mCreateInfo.colorBlendState.attachmentCount)
    delete[] mCreateInfo.colorBlendState.attachments;
  if (mCreateInfo.viewportState.viewportCount)
    delete[] mCreateInfo.dynamicState.dynamicStates;
}
//===========computePipeline
ComputePipeline::ComputePipeline(const ComputePipelineCreateInfo &createInfo)
    : mCreateInfo(createInfo) {
  auto &&e =
      const_cast<SpecializationInfo *>(&mCreateInfo.stage.specializationInfo);
  auto &&s = createInfo.stage.specializationInfo;
  e->constantIds = new uint32_t[e->constantValueCount];
  memcpy((void *)e->constantIds, s.constantIds,
         sizeof(uint32_t) * e->constantValueCount);
  e->constantValues = new uint32_t[e->constantValueCount];
  memcpy((void *)e->constantValues, s.constantValues,
         sizeof(uint32_t) * e->constantValueCount);
}
ComputePipeline::~ComputePipeline() {
  if (mCreateInfo.stage.specializationInfo.constantValueCount) {
    delete[] mCreateInfo.stage.specializationInfo.constantIds;
    delete[] mCreateInfo.stage.specializationInfo.constantValues;
  }
}
}  // namespace Shit
