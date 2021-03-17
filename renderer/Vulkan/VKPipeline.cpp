/**
 * @file VKPipeline.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "VKPipeline.h"
#include "VKDescriptor.h"
#include "VKShader.h"
#include "VKRenderPass.h"
namespace Shit
{

	VKPipelineLayout::VKPipelineLayout(VkDevice device, const PipelineLayoutCreateInfo &createInfo)
		: PipelineLayout(createInfo), mDevice(device)
	{
		std::vector<VkDescriptorSetLayout> setLayouts;
		std::vector<VkPushConstantRange> pushConstantRanges;
		for (auto &&e : createInfo.setLayouts)
			setLayouts.emplace_back(static_cast<VKDescriptorSetLayout *>(e)->GetHandle());
		for (auto &&e : createInfo.pushConstantRanges)
		{
			pushConstantRanges.emplace_back(
				VkPushConstantRange{
					static_cast<VkShaderStageFlags>(Map(e.stageFlags)),
					e.offset,
					e.size});
		}

		VkPipelineLayoutCreateInfo info{
			VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(setLayouts.size()),
			setLayouts.data(),
			static_cast<uint32_t>(pushConstantRanges.size()),
			pushConstantRanges.data()};

		if (vkCreatePipelineLayout(mDevice, &info, nullptr, &mHandle) != VK_SUCCESS)
			THROW("failed to create pipelinelayout");
	}

	VKGraphicsPipeline::VKGraphicsPipeline(VkDevice device, const GraphicsPipelineCreateInfo &createInfo)
		: GraphicsPipeline(createInfo), VKPipeline(device)
	{
		using constantVaule_T = std::decay_t<decltype(SpecializationInfo::constantValues)>::value_type;
		auto constantVaule_T_size = sizeof(constantVaule_T);
		auto stageCount = createInfo.stages.size();

		std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos(stageCount);
		std::vector<VkSpecializationInfo> specInfos(stageCount);
		std::vector<std::vector<VkSpecializationMapEntry>> entries(stageCount);

		for (size_t i = 0; i < stageCount; ++i)
		{
			auto &&shaderModuleInfo = createInfo.stages[i];
			shaderStageCreateInfos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shaderStageCreateInfos[i].stage = Map(shaderModuleInfo.stage);
			shaderStageCreateInfos[i].module = static_cast<VKShader *>(shaderModuleInfo.pShader)->GetHandle();
			shaderStageCreateInfos[i].pName = shaderModuleInfo.entryName;

			auto count = shaderModuleInfo.specializationInfo.constantIDs.size();
			entries[i].resize(count);
			for (size_t j = 0; j < count; ++j)
			{
				entries[i][j] = VkSpecializationMapEntry{shaderModuleInfo.specializationInfo.constantIDs[i],
														 static_cast<uint32_t>(i * constantVaule_T_size),
														 constantVaule_T_size};
			}
			specInfos[i].mapEntryCount = count;
			specInfos[i].dataSize = shaderModuleInfo.specializationInfo.constantValues.size() * constantVaule_T_size;
			specInfos[i].pData = shaderModuleInfo.specializationInfo.constantValues.data();
			specInfos[i].pMapEntries = entries[i].data();

			shaderStageCreateInfos[i].pSpecializationInfo = &specInfos[i];
		}

		uint32_t vertexInputBindingCount = createInfo.vertexInputState.vertexBindingDescriptions.size();
		std::vector<VkVertexInputBindingDescription> vertexInputBindingDescs(vertexInputBindingCount);
		for (uint32_t i = 0; i < vertexInputBindingCount; ++i)
		{
			vertexInputBindingDescs[i] = {
				i,
				createInfo.vertexInputState.vertexBindingDescriptions[i].stride,
				createInfo.vertexInputState.vertexBindingDescriptions[i].divisor ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX};
		}
		uint32_t vertexAttribCount = createInfo.vertexInputState.vertexAttributeDescriptions.size();
		std::vector<VkVertexInputAttributeDescription> vertexAttributes(vertexAttribCount);
		for (uint32_t i = 0; i < vertexAttribCount; ++i)
		{
			auto &&attrib = createInfo.vertexInputState.vertexAttributeDescriptions[i];
			vertexAttributes[i] = {
				attrib.location,
				attrib.binding,
				GetFormat(attrib.dataType, attrib.components, attrib.normalized),
				attrib.offset,
			};
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
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			nullptr,
			0,
			Map(createInfo.inputAssemblyState.topology),
			createInfo.inputAssemblyState.primitiveRestartEnable //primitive restart
		};
		VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			nullptr,
			createInfo.tessellationState.patchControlPoints};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(createInfo.viewportState.viewports.size()),
			reinterpret_cast<const VkViewport *>(createInfo.viewportState.viewports.data()),
			static_cast<uint32_t>(createInfo.viewportState.scissors.size()),
			reinterpret_cast<const VkRect2D *>(createInfo.viewportState.scissors.data())};

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			nullptr,
			0,
			createInfo.rasterizationState.depthClampEnable,
			createInfo.rasterizationState.rasterizerDiscardEnbale,
			Map(createInfo.rasterizationState.polygonMode),
			Map(createInfo.rasterizationState.cullMode),
			Map(createInfo.rasterizationState.frontFace),
			createInfo.rasterizationState.depthBiasEnable,
			createInfo.rasterizationState.depthBiasContantFactor,
			createInfo.rasterizationState.depthBiasClamp,
			createInfo.rasterizationState.depthBiasSlopeFactor,
			createInfo.rasterizationState.lineWidth};

		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<VkSampleCountFlagBits>(createInfo.multisampleState.rasterizationSamples),
			createInfo.multisampleState.sampleShadingEnable,
			createInfo.multisampleState.minSampleShading,
			createInfo.multisampleState.pSampleMask,
			createInfo.multisampleState.alphaToCoverageEnable,
			createInfo.multisampleState.alphaToOneEnable};
		VkStencilOpState frontStencilOpState{
			Map(createInfo.depthStencilState.front.failOp),
			Map(createInfo.depthStencilState.front.passOp),
			Map(createInfo.depthStencilState.front.depthFailOp),
			Map(createInfo.depthStencilState.front.compareOp),
			createInfo.depthStencilState.front.compareMask,
			createInfo.depthStencilState.front.writeMask,
			createInfo.depthStencilState.front.reference,
		};
		VkStencilOpState backStencilOpState{
			Map(createInfo.depthStencilState.back.failOp),
			Map(createInfo.depthStencilState.back.passOp),
			Map(createInfo.depthStencilState.back.depthFailOp),
			Map(createInfo.depthStencilState.back.compareOp),
			createInfo.depthStencilState.back.compareMask,
			createInfo.depthStencilState.back.writeMask,
			createInfo.depthStencilState.back.reference,
		};
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			nullptr,
			0,
			createInfo.depthStencilState.depthTestEnable,
			createInfo.depthStencilState.depthWriteEnable,
			Map(createInfo.depthStencilState.depthCompareOp),
			createInfo.depthStencilState.depthBoundsTestEnable,
			createInfo.depthStencilState.stencilTestEnable,
			std::move(frontStencilOpState),
			std::move(backStencilOpState),
			createInfo.depthStencilState.minDepthBounds,
			createInfo.depthStencilState.maxDepthBounds};

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;
		colorBlendAttachmentStates.reserve(createInfo.colorBlendState.attachments.size());
		for (auto &&e : createInfo.colorBlendState.attachments)
		{
			colorBlendAttachmentStates.emplace_back(
				VkPipelineColorBlendAttachmentState{e.blendEnable,
													Map(e.srcColorBlendFactor),
													Map(e.dstColorBlendFactor),
													Map(e.colorBlendOp),
													Map(e.srcAlphaBlendFactor),
													Map(e.dstAlphaBlendFactor),
													Map(e.alphaBlendOp),
													Map(e.colorWriteMask)});
		}
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			nullptr,
			0,
			createInfo.colorBlendState.logicOpEnable,
			Map(createInfo.colorBlendState.logicOp),
			static_cast<uint32_t>(colorBlendAttachmentStates.size()),
			colorBlendAttachmentStates.data(),
		};
		memcpy(colorBlendStateCreateInfo.blendConstants, createInfo.colorBlendState.blendConstants.data(), sizeof(float) * 4);

		std::vector<VkDynamicState> dynamicStates;
		dynamicStates.reserve(createInfo.dynamicState.dynamicStates.size());
		for (auto &&e : createInfo.dynamicState.dynamicStates)
		{
			dynamicStates.emplace_back(Map(e));
		}
		VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(dynamicStates.size()),
			dynamicStates.data()};

		VkGraphicsPipelineCreateInfo pipelineCreateInfo{
			VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			nullptr,
			0,
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
			createInfo.pLayout ? static_cast<VKPipelineLayout *>(createInfo.pLayout)->GetHandle() : VK_NULL_HANDLE,
			createInfo.pRenderPass ? static_cast<VKRenderPass *>(createInfo.pRenderPass)->GetHandle() : VK_NULL_HANDLE,
			createInfo.subpass, //subpass index in renderpass
		};
		std::vector<VkGraphicsPipelineCreateInfo> graphicsPipelineCreateInfos{pipelineCreateInfo};
		if (vkCreateGraphicsPipelines(mDevice,
									  VK_NULL_HANDLE,
									  static_cast<uint32_t>(graphicsPipelineCreateInfos.size()),
									  graphicsPipelineCreateInfos.data(),
									  nullptr,
									  &mHandle) != VK_SUCCESS)
			THROW("failed to create graphics pipeline");
	}
} // namespace Shit
