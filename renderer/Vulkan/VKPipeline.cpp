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
														 i * constantVaule_T_size,
														 constantVaule_T_size};
			}
			specInfos[i].mapEntryCount = count;
			specInfos[i].dataSize = shaderModuleInfo.specializationInfo.constantValues.size() * constantVaule_T_size;
			specInfos[i].pData = shaderModuleInfo.specializationInfo.constantValues.data();
			specInfos[i].pMapEntries = entries[i].data();

			shaderStageCreateInfos[i].pSpecializationInfo = &specInfos[i];
		}

		VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		};

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			VK_FALSE //primitive restart
		};
		VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			nullptr,
			0};

		VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			nullptr,
			0,
			static_cast<uint32_t>(createInfo.viewport.viewports.size()),
			reinterpret_cast<const VkViewport *>(createInfo.viewport.viewports.data()),
			static_cast<uint32_t>(createInfo.viewport.scissors.size()),
			reinterpret_cast<const VkRect2D *>(createInfo.viewport.scissors.data())};

		VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_FALSE,
			VK_FALSE,
			VK_POLYGON_MODE_FILL,
			VK_CULL_MODE_BACK_BIT,
			VK_FRONT_FACE_COUNTER_CLOCKWISE,
			VK_FALSE, //depth bias
			0,		  //depth bias constant factor
			0,		  //depth bias clamp
			0,		  //depth bias slope factor
			1		  //line width
		};
		VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FALSE, //disable sample shading
			1,		  //min sample shading, must be in range [0,1], can be ignored when sample shading is diabled ,
			nullptr,  //sample mask
			VK_FALSE,
			VK_FALSE, //
		};
		VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			nullptr,
			0};

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates{
			{VK_TRUE,
			 VK_BLEND_FACTOR_SRC_ALPHA,
			 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			 VK_BLEND_OP_ADD,
			 VK_BLEND_FACTOR_SRC_ALPHA,
			 VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			 VK_BLEND_OP_ADD,
			 VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT}};
		VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{
			VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			nullptr,
			0,
			VK_TRUE,
			VK_LOGIC_OP_COPY,
			static_cast<uint32_t>(colorBlendAttachmentStates.size()),
			colorBlendAttachmentStates.data(),
			{0, 0, 0, 0} //blend constant
		};
		std::vector<VkDynamicState> dynamicStates{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH};
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
			nullptr, //&dynamicStateCreateInfo,
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
