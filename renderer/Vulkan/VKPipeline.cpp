/**
 * @file VKPipeline.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKPipeline.hpp"

#include "VKDescriptor.hpp"
#include "VKRenderPass.hpp"
#include "VKShader.hpp"
namespace Shit {

VKPipelineLayout::VKPipelineLayout(VKDevice *device, const PipelineLayoutCreateInfo &createInfo)
    : PipelineLayout(createInfo), VKDeviceObject(device) {
    std::vector<VkDescriptorSetLayout> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
    for (uint32_t i = 0; i < mCreateInfo.setLayoutCount; ++i) {
        if (mCreateInfo.pSetLayouts[i])
            setLayouts.emplace_back(
                static_cast<const VKDescriptorSetLayout *>(mCreateInfo.pSetLayouts[i])->GetHandle());
        else
            setLayouts.emplace_back(VK_NULL_HANDLE);
    }
    for (uint32_t i = 0; i < mCreateInfo.pushConstantRangeCount; ++i) {
        auto &&e = mCreateInfo.pPushConstantRanges[i];
        pushConstantRanges.emplace_back(
            VkPushConstantRange{static_cast<VkShaderStageFlags>(Map(e.stageFlags)), e.offset, e.size});
    }

    VkPipelineLayoutCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                                    nullptr,
                                    0,
                                    static_cast<uint32_t>(setLayouts.size()),
                                    setLayouts.data(),
                                    static_cast<uint32_t>(pushConstantRanges.size()),
                                    pushConstantRanges.data()};

    if (vkCreatePipelineLayout(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create pipelinelayout");
}

//=================================VKPipeline=======================
void VKPipeline::ConvertPipelineShaderStageInfo(uint32_t stageCount, PipelineShaderStageCreateInfo const *stages,
                                                std::vector<VkPipelineShaderStageCreateInfo> &dst) {
    uint32_t constantVaule_T_size = 4;

    dst.resize(stageCount);
    std::vector<VkSpecializationInfo> specInfos(stageCount);
    std::vector<std::vector<VkSpecializationMapEntry>> entries(stageCount);

    for (uint32_t i = 0; i < stageCount; ++i) {
        auto &&shaderModuleInfo = stages[i];
        if (shaderModuleInfo.specializationInfo.constantValueCount) {
            entries[i].resize(shaderModuleInfo.specializationInfo.constantValueCount);
            for (uint32_t j = 0; j < shaderModuleInfo.specializationInfo.constantValueCount; ++j) {
                entries[i][j] = VkSpecializationMapEntry{shaderModuleInfo.specializationInfo.constantIds[j],
                                                         j * constantVaule_T_size, constantVaule_T_size};
            }
        }
        specInfos[i].mapEntryCount = shaderModuleInfo.specializationInfo.constantValueCount;
        specInfos[i].dataSize = shaderModuleInfo.specializationInfo.constantValueCount * constantVaule_T_size;
        specInfos[i].pData = shaderModuleInfo.specializationInfo.constantValues;
        specInfos[i].pMapEntries = entries[i].data();

        dst[i] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                  nullptr,
                  0,
                  Map(shaderModuleInfo.stage),
                  static_cast<VKShader const *>(shaderModuleInfo.pShader)->GetHandle(),
                  shaderModuleInfo.entryName,
                  &specInfos[i]};
    }
}
//=================================VKGraphicsPipeline=======================
void VKGraphicsPipeline::Initialize() {
    // using constantVaule_T =
    // std::decay_t<decltype(SpecializationInfo::constantValues)>::value_type;
    // auto constantVaule_T_size = sizeof(constantVaule_T);
    // auto stageCount = mCreateInfo.stages
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    // ConvertPipelineShaderStageInfo(mCreateInfo.stageCount, mCreateInfo.stages,
    // shaderStageCreateInfos);

    uint32_t constantVaule_T_size = 4;

    shaderStageCreateInfos.resize(mCreateInfo.stageCount);
    std::vector<VkSpecializationInfo> specInfos(mCreateInfo.stageCount);
    std::vector<std::vector<VkSpecializationMapEntry>> entries(mCreateInfo.stageCount);

    for (uint32_t i = 0; i < mCreateInfo.stageCount; ++i) {
        auto &&shaderModuleInfo = mCreateInfo.stages[i];
        if (shaderModuleInfo.specializationInfo.constantValueCount) {
            entries[i].resize(shaderModuleInfo.specializationInfo.constantValueCount);
            for (uint32_t j = 0; j < shaderModuleInfo.specializationInfo.constantValueCount; ++j) {
                entries[i][j] = VkSpecializationMapEntry{shaderModuleInfo.specializationInfo.constantIds[j],
                                                         j * constantVaule_T_size, constantVaule_T_size};
            }
        }
        specInfos[i].mapEntryCount = shaderModuleInfo.specializationInfo.constantValueCount;
        specInfos[i].dataSize = shaderModuleInfo.specializationInfo.constantValueCount * constantVaule_T_size;
        specInfos[i].pData = shaderModuleInfo.specializationInfo.constantValues;
        specInfos[i].pMapEntries = entries[i].data();

        shaderStageCreateInfos[i] = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                                     nullptr,
                                     0,
                                     Map(shaderModuleInfo.stage),
                                     static_cast<VKShader const *>(shaderModuleInfo.pShader)->GetHandle(),
                                     shaderModuleInfo.entryName,
                                     &specInfos[i]};
    }

    uint32_t vertexInputBindingCount = mCreateInfo.vertexInputState.vertexBindingDescriptionCount;
    std::vector<VkVertexInputBindingDescription> vertexInputBindingDescs;
    vertexInputBindingDescs.reserve(vertexInputBindingCount);
    for (uint32_t i = 0; i < vertexInputBindingCount; ++i) {
        auto &&e = mCreateInfo.vertexInputState.vertexBindingDescriptions[i];
        vertexInputBindingDescs.emplace_back(e.binding, e.stride,
                                             e.divisor ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);
    }
    uint32_t vertexAttribCount = mCreateInfo.vertexInputState.vertexAttributeDescriptionCount;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    vertexAttributes.reserve(vertexAttribCount);
    for (uint32_t i = 0; i < vertexAttribCount; ++i) {
        auto &&attrib = mCreateInfo.vertexInputState.vertexAttributeDescriptions[i];
        vertexAttributes.emplace_back(attrib.location, attrib.binding, Map(attrib.format), attrib.offset);
    }
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        nullptr,
        0,
        vertexInputBindingCount,
        vertexInputBindingDescs.data(),
        vertexAttribCount,
        vertexAttributes.data()};
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO, nullptr, 0,
        Map(mCreateInfo.inputAssemblyState.topology),
        mCreateInfo.inputAssemblyState.primitiveRestartEnable  // primitive restart
    };
    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO, nullptr, 0,
        mCreateInfo.tessellationState.patchControlPoints};

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        nullptr,
        0,
        mCreateInfo.rasterizationState.depthClampEnable,
        mCreateInfo.rasterizationState.rasterizerDiscardEnbale,
        Map(mCreateInfo.rasterizationState.polygonMode),
        Map(mCreateInfo.rasterizationState.cullMode),
        Map(mCreateInfo.rasterizationState.frontFace),
        mCreateInfo.rasterizationState.depthBiasEnable,
        mCreateInfo.rasterizationState.depthBiasContantFactor,
        mCreateInfo.rasterizationState.depthBiasClamp,
        mCreateInfo.rasterizationState.depthBiasSlopeFactor,
        mCreateInfo.rasterizationState.lineWidth};

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        nullptr,
        0,
        static_cast<VkSampleCountFlagBits>(mCreateInfo.multisampleState.rasterizationSamples),
        mCreateInfo.multisampleState.sampleShadingEnable,
        mCreateInfo.multisampleState.minSampleShading,
        mCreateInfo.multisampleState.pSampleMask,
        mCreateInfo.multisampleState.alphaToCoverageEnable,
        mCreateInfo.multisampleState.alphaToOneEnable};
    VkStencilOpState frontStencilOpState{
        Map(mCreateInfo.depthStencilState.front.failOp),      Map(mCreateInfo.depthStencilState.front.passOp),
        Map(mCreateInfo.depthStencilState.front.depthFailOp), Map(mCreateInfo.depthStencilState.front.compareOp),
        mCreateInfo.depthStencilState.front.compareMask,      mCreateInfo.depthStencilState.front.writeMask,
        mCreateInfo.depthStencilState.front.reference,
    };
    VkStencilOpState backStencilOpState{
        Map(mCreateInfo.depthStencilState.back.failOp),      Map(mCreateInfo.depthStencilState.back.passOp),
        Map(mCreateInfo.depthStencilState.back.depthFailOp), Map(mCreateInfo.depthStencilState.back.compareOp),
        mCreateInfo.depthStencilState.back.compareMask,      mCreateInfo.depthStencilState.back.writeMask,
        mCreateInfo.depthStencilState.back.reference,
    };
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        nullptr,
        0,
        mCreateInfo.depthStencilState.depthTestEnable,
        mCreateInfo.depthStencilState.depthWriteEnable,
        Map(mCreateInfo.depthStencilState.depthCompareOp),
        mCreateInfo.depthStencilState.depthBoundsTestEnable,
        mCreateInfo.depthStencilState.stencilTestEnable,
        std::move(frontStencilOpState),
        std::move(backStencilOpState),
        mCreateInfo.depthStencilState.minDepthBounds,
        mCreateInfo.depthStencilState.maxDepthBounds};

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
    colorBlendAttachmentStates.reserve(mCreateInfo.colorBlendState.attachmentCount);
    for (uint32_t i = 0; i < mCreateInfo.colorBlendState.attachmentCount; ++i) {
        auto &&e = mCreateInfo.colorBlendState.attachments[i];
        colorBlendAttachmentStates.emplace_back(VkPipelineColorBlendAttachmentState{
            e.blendEnable, Map(e.srcColorBlendFactor), Map(e.dstColorBlendFactor), Map(e.colorBlendOp),
            Map(e.srcAlphaBlendFactor), Map(e.dstAlphaBlendFactor), Map(e.alphaBlendOp), Map(e.colorWriteMask)});
    }
    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        nullptr,
        0,
        mCreateInfo.colorBlendState.logicOpEnable,
        Map(mCreateInfo.colorBlendState.logicOp),
        static_cast<uint32_t>(colorBlendAttachmentStates.size()),
        colorBlendAttachmentStates.data()};
    memcpy(colorBlendStateCreateInfo.blendConstants, mCreateInfo.colorBlendState.blendConstants, sizeof(float) * 4);

    bool dyanmicStateViewportWidthCount = false, dyanmicStateScissorWidthCount = false;

    std::vector<VkDynamicState> dynamicStates(mCreateInfo.dynamicState.dynamicStateCount);
    for (uint32_t i = 0; i < mCreateInfo.dynamicState.dynamicStateCount; ++i) {
        dynamicStates[i] = Map(mCreateInfo.dynamicState.dynamicStates[i]);
        dyanmicStateViewportWidthCount |= (dynamicStates[i] == VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT);
        dyanmicStateScissorWidthCount |= (dynamicStates[i] == VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT);
    }

    uint32_t viewportCount = mCreateInfo.viewportState.viewportCount;
    std::vector<VkViewport> viewports(viewportCount);
    for (uint32_t i = 0; i < viewportCount; ++i) {
        viewports[i] = {
            mCreateInfo.viewportState.viewports[i].x,
            mCreateInfo.viewportState.viewports[i].height - mCreateInfo.viewportState.viewports[i].y,
            mCreateInfo.viewportState.viewports[i].width,
            -mCreateInfo.viewportState.viewports[i].height,
            mCreateInfo.viewportState.viewports[i].minDepth,
            mCreateInfo.viewportState.viewports[i].maxDepth,
        };
    }

    uint32_t scissorCount = mCreateInfo.viewportState.scissorCount;
    std::vector<VkRect2D> scissors(scissorCount);

    for (uint32_t i = 0; i < scissorCount; ++i) {
        scissors[i] = {
            {mCreateInfo.viewportState.scissors[i].offset.x, mCreateInfo.viewportState.scissors[i].offset.y},
            {mCreateInfo.viewportState.scissors[i].extent.width, mCreateInfo.viewportState.scissors[i].extent.height}};
    }

    if (dyanmicStateViewportWidthCount)
        viewportCount = 0;
    else if (viewportCount == 0) {
        viewportCount = 1;
        viewports = {{}};
    }
    if (dyanmicStateScissorWidthCount)
        scissorCount = 0;
    else if (scissorCount == 0) {
        scissorCount = 1;
        scissors = {{}};
    }

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
                                                            nullptr, 0, static_cast<uint32_t>(dynamicStates.size()),
                                                            dynamicStates.data()};

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
                                                              nullptr,
                                                              0,
                                                              viewportCount,
                                                              viewports.data(),
                                                              scissorCount,
                                                              scissors.data()};

    VkGraphicsPipelineCreateInfo pipelineCreateInfo{
        VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        nullptr,
        0,  // Map(mCreateInfo.flags),
        static_cast<uint32_t>(shaderStageCreateInfos.size()),
        shaderStageCreateInfos.data(),
        &vertexInputStateCreateInfo,
        &inputAssemblyStateCreateInfo,
        &tessellationStateCreateInfo,
        &viewportStateCreateInfo,
        &rasterizationStateCreateInfo,
        &multisampleStateCreateInfo,
        &depthStencilStateCreateInfo,
        &colorBlendStateCreateInfo,
        &dynamicStateCreateInfo,
        mCreateInfo.pLayout ? static_cast<VKPipelineLayout const *>(mCreateInfo.pLayout)->GetHandle() : VK_NULL_HANDLE,
        mCreateInfo.pRenderPass ? static_cast<VKRenderPass const *>(mCreateInfo.pRenderPass)->GetHandle()
                                : VK_NULL_HANDLE,
        mCreateInfo.subpass,  // subpass index in renderpass
    };
    CHECK_VK_RESULT(
        vkCreateGraphicsPipelines(mpDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mHandle))
}
VKGraphicsPipeline::VKGraphicsPipeline(VKDevice *device, const GraphicsPipelineCreateInfo &createInfo)
    : GraphicsPipeline(createInfo), VKPipeline(device) {
    Initialize();
}
//=======================================================
VKComputePipeline::VKComputePipeline(VKDevice *device, const ComputePipelineCreateInfo &createInfo)
    : ComputePipeline(createInfo), VKPipeline(device) {
    Initialize();
}
void VKComputePipeline::Initialize() {
    uint32_t constantVaule_T_size = 4;
    VkSpecializationInfo specInfo;
    std::vector<VkSpecializationMapEntry> entries;

    auto &&shaderModuleInfo = mCreateInfo.stage;
    if (shaderModuleInfo.specializationInfo.constantValueCount) {
        entries.resize(shaderModuleInfo.specializationInfo.constantValueCount);
        for (uint32_t j = 0; j < shaderModuleInfo.specializationInfo.constantValueCount; ++j) {
            entries[j] = VkSpecializationMapEntry{shaderModuleInfo.specializationInfo.constantIds[j],
                                                  j * constantVaule_T_size, constantVaule_T_size};
        }
    }
    specInfo.mapEntryCount = shaderModuleInfo.specializationInfo.constantValueCount;
    specInfo.dataSize = shaderModuleInfo.specializationInfo.constantValueCount * constantVaule_T_size;
    specInfo.pData = shaderModuleInfo.specializationInfo.constantValues;
    specInfo.pMapEntries = entries.data();

    VkPipelineShaderStageCreateInfo stageCreateInfo{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        nullptr,
        0,
        Map(shaderModuleInfo.stage),
        static_cast<VKShader const *>(shaderModuleInfo.pShader)->GetHandle(),
        shaderModuleInfo.entryName,
        &specInfo};

    VkComputePipelineCreateInfo info{
        VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        nullptr,
        {},
        stageCreateInfo,
        static_cast<VKPipelineLayout const *>(mCreateInfo.pLayout)->GetHandle(),
        VK_NULL_HANDLE,  // base pipeline handle
        0                // base pipeline index
    };
    CHECK_VK_RESULT(vkCreateComputePipelines(mpDevice->GetHandle(),
                                             VK_NULL_HANDLE,  // pipeline cache
                                             1, &info, nullptr, &mHandle))
}
}  // namespace Shit
