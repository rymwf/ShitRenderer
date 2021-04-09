/**
 * @file GLCommandBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLCommandBuffer.hpp"
#include "GLFramebuffer.hpp"
#include "GLRenderPass.hpp"
#include "GLPipeline.hpp"
#include "GLBuffer.hpp"
#include "GLDescriptor.hpp"
#include "GLImage.hpp"
namespace Shit
{
	GLCommandBuffer::GLCommandBuffer(GLStateManager *pStateManager, const CommandBufferCreateInfo &createInfo)
		: CommandBuffer(createInfo), mpStateManager(pStateManager)
	{
	}

	void GLCommandBuffer::ClearBuffer()
	{
		auto &&attachments = mCurRenderPass->GetCreateInfoPtr()->attachments;
		auto &&subpassDesc = mCurRenderPass->GetCreateInfoPtr()->subpasses[mCurSubpass];
		for (auto &&e : subpassDesc.colorAttachments)
		{
			if (attachments[e.attachment].loadOp != AttachmentLoadOp::CLEAR)
				continue;
			auto clearColorValue = std::get<ClearColorValue>(*static_cast<GLRenderPass *>(mCurRenderPass)->GetAttachmentClearValuePtr(e.attachment));
			if (auto pVal = std::get_if<std::array<float, 4>>(&clearColorValue))
			{
				glClearBufferfv(GL_COLOR, e.attachment, pVal->data());
				continue;
			}
			if (auto pVal = std::get_if<std::array<int32_t, 4>>(&clearColorValue))
			{
				glClearBufferiv(GL_COLOR, e.attachment, pVal->data());
				continue;
			}
			if (auto pVal = std::get_if<std::array<uint32_t, 4>>(&clearColorValue))
			{
				glClearBufferuiv(GL_COLOR, e.attachment, pVal->data());
				continue;
			}
		}
		if (subpassDesc.depthStencilAttachment.has_value())
		{
			auto clearDepthStencilValue = std::get<ClearDepthStencilValue>(*static_cast<GLRenderPass *>(mCurRenderPass)->GetAttachmentClearValuePtr(subpassDesc.depthStencilAttachment->attachment));
			if (attachments[subpassDesc.depthStencilAttachment->attachment].loadOp == AttachmentLoadOp::CLEAR)
				glClearBufferfv(GL_DEPTH, 0, &clearDepthStencilValue.depth);
			if (attachments[subpassDesc.depthStencilAttachment->attachment].stencilLoadOp == AttachmentLoadOp::CLEAR)
				glClearBufferuiv(GL_STENCIL, 0, &clearDepthStencilValue.stencil);
		}
	}

	size_t GLCommandBuffer::ExecuteCommand(GLCommandCode commandCode, const void *pCur)
	{
		switch (commandCode)
		{
		case GLCommandCode::BeginRenderPass:
		{
			auto cmd = reinterpret_cast<const RenderPassBeginInfo *>(pCur);
			mpCurFramebuffer = static_cast<GLFramebuffer *>(cmd->pFramebuffer);
			mpStateManager->BindDrawFramebuffer(mpCurFramebuffer->GetRenderFramebuffer());
			mCurRenderPass = cmd->pRenderPass;
			mCurSubpass = 0;
			//bind attachments
			mpCurFramebuffer->SetRenderFBOAttachment(mCurSubpass);
			//TODO: what is renderArea
			//clear value
			ClearBuffer();
			return sizeof(*cmd);
		}
		case GLCommandCode::BindIndexBuffer:
		{
			auto cmd = reinterpret_cast<const BindIndexBufferInfo *>(pCur);
			mpStateManager->BindIndexBuffer(static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle(), Map(cmd->indexType), cmd->offset);
			mCurIndexType = cmd->indexType;
			return sizeof(*cmd);
		}
		case GLCommandCode::BindPipeline:
		{
			auto cmd = reinterpret_cast<const BindPipelineInfo *>(pCur);
			mCurPipeline = cmd->pPipeline;
			GLPipeline *pPipeline = dynamic_cast<GLPipeline *>(cmd->pPipeline);
			if (pPipeline)
			{
				mpStateManager->BindPipeline(pPipeline->GetHandle());
				if (cmd->bindPoint == PipelineBindPoint::GRAPHICS)
				{
					auto pGraphicsPipeline = static_cast<GLGraphicsPipeline *>(pPipeline);
					auto &&pGraphicsPipelineCreateInfo = pGraphicsPipeline->GetCreateInfoPtr();

					mpStateManager->BindVertexArray(pGraphicsPipeline->GetVertexArray());

					//primitive
					mpStateManager->PrimitiveTopology(Map(pGraphicsPipelineCreateInfo->inputAssemblyState.topology));
					if (pGraphicsPipelineCreateInfo->inputAssemblyState.primitiveRestartEnable)
						mpStateManager->EnablePrimitiveRestart();
					else
						mpStateManager->DisablePrimitiveRestart();

					//viewport
					GLuint count = static_cast<GLuint>(pGraphicsPipelineCreateInfo->viewportState.viewports.size());
					std::vector<float> viewportData(count * 6);
					memcpy(viewportData.data(), pGraphicsPipelineCreateInfo->viewportState.viewports.data(), count * 6 * sizeof(float));
					mpStateManager->SetViewports(0, count, viewportData.data());

					//scissor
					count = static_cast<GLuint>(pGraphicsPipelineCreateInfo->viewportState.scissors.size());
					if (count == 0)
					{
						mpStateManager->DisableCapability(GL_SCISSOR_TEST);
					}
					else
					{
						mpStateManager->EnableCapability(GL_SCISSOR_TEST);
						std::vector<int32_t> scissorsData(count * 4);
						memcpy(scissorsData.data(), pGraphicsPipelineCreateInfo->viewportState.scissors.data(), count * 4 * sizeof(int32_t));
						mpStateManager->SetScissors(0, count, scissorsData.data());
					}

					//rasterization
					if (pGraphicsPipelineCreateInfo->rasterizationState.depthClampEnable)
						mpStateManager->EnableCapability(GL_DEPTH_CLAMP); //no z clip
					else
						mpStateManager->DisableCapability(GL_DEPTH_CLAMP);

					if (pGraphicsPipelineCreateInfo->rasterizationState.rasterizerDiscardEnbale)
						mpStateManager->EnableCapability(GL_RASTERIZER_DISCARD);
					else
						mpStateManager->DisableCapability(GL_RASTERIZER_DISCARD);

					mpStateManager->PolygonMode(Map(pGraphicsPipelineCreateInfo->rasterizationState.polygonMode));
					if (pGraphicsPipelineCreateInfo->rasterizationState.cullMode == CullMode::NONE)
						mpStateManager->DisableCapability(GL_CULL_FACE);
					else
					{
						mpStateManager->EnableCapability(GL_CULL_FACE);
						mpStateManager->CullFace(Map(pGraphicsPipelineCreateInfo->rasterizationState.cullMode));
					}
					mpStateManager->FrontFace(Map(pGraphicsPipelineCreateInfo->rasterizationState.frontFace));

					//depth biase
					if (pGraphicsPipelineCreateInfo->rasterizationState.depthBiasEnable)
					{
						if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::FILL)
							mpStateManager->EnableCapability(GL_POLYGON_OFFSET_FILL);
						else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::LINE)
							mpStateManager->EnableCapability(GL_POLYGON_OFFSET_LINE);
						else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::POINT)
							mpStateManager->EnableCapability(GL_POLYGON_OFFSET_POINT);

						mpStateManager->PolygonOffSetClamp(
							pGraphicsPipelineCreateInfo->rasterizationState.depthBiasContantFactor,
							pGraphicsPipelineCreateInfo->rasterizationState.depthBiasSlopeFactor,
							pGraphicsPipelineCreateInfo->rasterizationState.depthBiasClamp);
					}
					else
					{
						if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::FILL)
							mpStateManager->DisableCapability(GL_POLYGON_OFFSET_FILL);
						else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::LINE)
							mpStateManager->DisableCapability(GL_POLYGON_OFFSET_LINE);
						else if (pGraphicsPipelineCreateInfo->rasterizationState.polygonMode == PolygonMode::POINT)
							mpStateManager->DisableCapability(GL_POLYGON_OFFSET_POINT);
						mpStateManager->PolygonOffSetClamp(0.f, 0.f, 0.f);
					}
					//line width
					mpStateManager->LineWidth(pGraphicsPipelineCreateInfo->rasterizationState.lineWidth);
					//multisample
					if (pGraphicsPipelineCreateInfo->multisampleState.sampleShadingEnable)
					{
						mpStateManager->EnableCapability(GL_SAMPLE_SHADING);
						mpStateManager->MinSampleShading(pGraphicsPipelineCreateInfo->multisampleState.minSampleShading);
						//TODO: sample mask
					}
					else
					{
						mpStateManager->DisableCapability(GL_SAMPLE_SHADING);
					}

					if (pGraphicsPipelineCreateInfo->multisampleState.alphaToCoverageEnable)
						mpStateManager->EnableCapability(GL_SAMPLE_ALPHA_TO_COVERAGE);
					else
						mpStateManager->DisableCapability(GL_SAMPLE_ALPHA_TO_COVERAGE);

					if (pGraphicsPipelineCreateInfo->multisampleState.alphaToOneEnable)
						mpStateManager->EnableCapability(GL_SAMPLE_ALPHA_TO_ONE);
					else
						mpStateManager->DisableCapability(GL_SAMPLE_ALPHA_TO_ONE);

					//depth test
					if (pGraphicsPipelineCreateInfo->depthStencilState.depthTestEnable)
					{
						mpStateManager->EnableCapability(GL_DEPTH_TEST);
						mpStateManager->DepthFunc(Map(pGraphicsPipelineCreateInfo->depthStencilState.depthCompareOp));
						mpStateManager->DepthMask(pGraphicsPipelineCreateInfo->depthStencilState.depthWriteEnable);
					}
					else
						mpStateManager->DisableCapability(GL_DEPTH_TEST);
					//stencil test
					if (pGraphicsPipelineCreateInfo->depthStencilState.stencilTestEnable)
					{
						mpStateManager->EnableCapability(GL_STENCIL_TEST);
						//stencil op
						mpStateManager->StencilOpState(
							GL_FRONT,
							Map(pGraphicsPipelineCreateInfo->depthStencilState.front.compareOp),
							pGraphicsPipelineCreateInfo->depthStencilState.front.reference,
							pGraphicsPipelineCreateInfo->depthStencilState.front.compareMask,
							Map(pGraphicsPipelineCreateInfo->depthStencilState.front.failOp),
							Map(pGraphicsPipelineCreateInfo->depthStencilState.front.depthFailOp),
							Map(pGraphicsPipelineCreateInfo->depthStencilState.front.passOp),
							pGraphicsPipelineCreateInfo->depthStencilState.front.writeMask);
						mpStateManager->StencilOpState(
							GL_BACK,
							Map(pGraphicsPipelineCreateInfo->depthStencilState.back.compareOp),
							pGraphicsPipelineCreateInfo->depthStencilState.back.reference,
							pGraphicsPipelineCreateInfo->depthStencilState.back.compareMask,
							Map(pGraphicsPipelineCreateInfo->depthStencilState.back.failOp),
							Map(pGraphicsPipelineCreateInfo->depthStencilState.back.depthFailOp),
							Map(pGraphicsPipelineCreateInfo->depthStencilState.back.passOp),
							pGraphicsPipelineCreateInfo->depthStencilState.back.writeMask);
					}
					else
						mpStateManager->DisableCapability(GL_STENCIL_TEST);

					//blend
					GLuint i = 0;
					for (auto &&e : pGraphicsPipelineCreateInfo->colorBlendState.attachments)
					{
						if (e.blendEnable)
						{
							mpStateManager->EnableBlend(i);
							mpStateManager->BlendEquation(i, Map(e.colorBlendOp), Map(e.alphaBlendOp));
							mpStateManager->BlendFunc(i,
													  Map(e.srcColorBlendFactor),
													  Map(e.dstColorBlendFactor),
													  Map(e.srcAlphaBlendFactor),
													  Map(e.dstAlphaBlendFactor));
						}
						else
						{
							mpStateManager->DisableBlend(i);
						}
						mpStateManager->ColorMask(i, static_cast<GLuint>(e.colorWriteMask));
						++i;
					}
					if (pGraphicsPipelineCreateInfo->colorBlendState.logicOpEnable)
					{
						mpStateManager->EnableCapability(GL_COLOR_LOGIC_OP);
						mpStateManager->LogicOp(Map(pGraphicsPipelineCreateInfo->colorBlendState.logicOp));
					}
					else
						mpStateManager->DisableCapability(GL_COLOR_LOGIC_OP);
					mpStateManager->BlendColor(pGraphicsPipelineCreateInfo->colorBlendState.blendConstants);
				}
			}
			else
			{
				mpStateManager->BindPipeline(0);
				if (cmd->bindPoint == PipelineBindPoint::GRAPHICS)
				{
					mpStateManager->BindVertexArray(0);
				}
			}

			return sizeof(*cmd);
		}
		case GLCommandCode::BindVertexBuffer:
		{
			//TODO: optimize???
			auto cmd = reinterpret_cast<const BindVertexBufferInfo *>(pCur);
			auto ppBuffers = reinterpret_cast<Buffer *const *>(&cmd->ppBuffers);
			auto pOffsets = reinterpret_cast<const uint64_t *>(&cmd->ppBuffers + cmd->bindingCount);
			//auto graphicsPipeline = ;
			auto &&vertexInputState = dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->vertexInputState;

			std::vector<GLuint> vertexBuffers(cmd->bindingCount);
			std::vector<GLintptr> offsets(cmd->bindingCount);
			std::vector<GLsizei> strides(cmd->bindingCount);
			for(auto&& bindingDesc:vertexInputState.vertexBindingDescriptions)
			{
				vertexBuffers[bindingDesc.binding] = static_cast<const GLBuffer *>(ppBuffers[bindingDesc.binding])->GetHandle();
				offsets[bindingDesc.binding] = pOffsets[bindingDesc.binding];
				strides[bindingDesc.binding] = bindingDesc.stride;
			}
			mpStateManager->BindVertexBuffer(cmd->firstBinding,
											 cmd->bindingCount,
											 vertexBuffers,
											 offsets,
											 strides);
			//do not render
			//for (auto &&attrib : vertexInputState.vertexAttributeDescriptions)
			//{
			//	auto index = attrib.binding + cmd->firstBinding;
			//	mpStateManager->BindBuffer(GL_ARRAY_BUFFER, static_cast<GLBuffer *>(&pBuffers[index])->GetHandle());
			//	uint32_t offset = static_cast<uint32_t>(attrib.offset + pOffsets[index]);
			//	glVertexAttribPointer(
			//		attrib.location,
			//		attrib.components,
			//		Map(attrib.dataType),
			//		attrib.normalized,
			//		vertexInputState.vertexBindingDescriptions[attrib.binding].stride,
			//		&offset);
			//	glVertexAttribDivisor(attrib.location, vertexInputState.vertexBindingDescriptions[attrib.binding].divisor);
			//	glEnableVertexAttribArray(attrib.location);
			//}
			return sizeof(BindVertexBufferInfo::bindingCount) +
				   sizeof(BindVertexBufferInfo::firstBinding) +
				   (sizeof(uint64_t) + sizeof(Buffer *)) * cmd->bindingCount;
		}
		case GLCommandCode::BindDescriptorSets:
		{
			auto cmd = reinterpret_cast<const BindDescriptorSetsInfo *>(pCur);
			auto pDescriptorSets = reinterpret_cast<DescriptorSet *const *>(&cmd->ppDescriptorSets);
			for (uint32_t i = 0; i < cmd->descriptorSetCount; ++i)
			{
				auto &&bindingAttributes = *(static_cast<const GLDescriptorSet *>(pDescriptorSets[i])->GetBindingAttributePtr());
				for (GLuint k = 0, len = static_cast<GLuint>(bindingAttributes.size()); k < len; ++k)
				{
					auto descriptorType = bindingAttributes[k].type;
					std::visit(
						overloaded{
							[&](const ImageView *pImageView) {
								if (!pImageView)
									return;
								if (descriptorType == DescriptorType::COMBINED_IMAGE_SAMPLER)
								{
									mpStateManager->BindTextureUnit(k,
																	Map(pImageView->GetCreateInfoPtr()->viewType, pImageView->GetCreateInfoPtr()->pImage->GetCreateInfoPtr()->samples),
																	static_cast<const GLImageView *>(pImageView)->GetHandle());
								}
								else if (descriptorType == DescriptorType::STORAGE_IMAGE)
								{
									mpStateManager->BindImageTexture(k,
																	 static_cast<const GLImageView *>(pImageView)->GetHandle(),
																	 0,
																	 GL_FALSE,
																	 0,
																	 GL_READ_WRITE,
																	 MapInternalFormat(pImageView->GetCreateInfoPtr()->format));
								}
							},
							[&](const DescriptorBufferInfo &bufferInfo) {
								if (!bufferInfo.pBuffer)
									return;
								GLenum target{};
								if (descriptorType == DescriptorType::UNIFORM_BUFFER)
									target = GL_UNIFORM_BUFFER;
								else if (descriptorType == DescriptorType::STORAGE_BUFFER)
									target = GL_SHADER_STORAGE_BUFFER;
								else
									THROW("invalid descriptor type");
								mpStateManager->BindBufferRange(target,
																k,
																static_cast<GLBuffer *>(bufferInfo.pBuffer)->GetHandle(),
																bufferInfo.offset,
																bufferInfo.range);
							},
							[](const BufferView *pBufferView) {
								if (!pBufferView)
									return;
							},
							[](std::monostate) {},
						},
						bindingAttributes[k].val);
				}
			}
			//TODO: dynamic offsets
			//auto pDynamicOffsetCount = reinterpret_cast<uint32_t *>(&pDescriptorSets[cmd->descriptorSetCount]);
			//auto pDynamicOffsets = reinterpret_cast<const uint32_t *>(&pDynamicOffsetCount + 1);

			//return sizeof(*cmd) + cmd->descriptorSetCount * sizeof(DescriptorSet *) + cmd->dynamicOffsetCount * sizeof(uint32_t *) - sizeof(DescriptorSet **) - sizeof(uint32_t *);
			return sizeof(PipelineBindPoint) * 2 +
				   sizeof(PipelineLayout *) +
				   sizeof(uint32_t) * 3 +
				   sizeof(DescriptorSet *) * cmd->descriptorSetCount +
				   sizeof(uint32_t) * cmd->dynamicOffsetCount;
		}
		case GLCommandCode::BlitImage:
		{
			auto cmd = reinterpret_cast<const BlitImageInfo *>(pCur);
			return sizeof(*cmd);
		}
		case GLCommandCode::CopyBuffer:
		{
			auto cmd = reinterpret_cast<const CopyBufferInfo *>(pCur);
			mpStateManager->BindBuffer(GL_COPY_READ_BUFFER, static_cast<GLBuffer *>(cmd->pSrcBuffer)->GetHandle());
			mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLBuffer *>(cmd->pDstBuffer)->GetHandle());
			for (uint32_t i = 0; i < cmd->regionCount; ++i)
			{
				glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, cmd->pRegions[i].srcOffset, cmd->pRegions[i].dstOffset, cmd->pRegions[i].size);
			}
			return sizeof(sizeof(Buffer *) * 2 + sizeof(uint32_t) + sizeof(BufferCopy) * cmd->regionCount);
		}
		case GLCommandCode::CopyBufferToImage:
		{
			auto cmd = reinterpret_cast<const CopyBufferToImageInfo *>(pCur);
			return sizeof(*cmd);
		}
		case GLCommandCode::CopyImage:
		{
			auto cmd = reinterpret_cast<const CopyImageInfo *>(pCur);
			return sizeof(*cmd);
		}
		case GLCommandCode::CopyImageToBuffer:
		{
			auto cmd = reinterpret_cast<const CopyImageToBufferInfo *>(pCur);
			return sizeof(*cmd);
		}
		case GLCommandCode::Draw:
		{
			auto cmd = reinterpret_cast<const DrawIndirectCommand *>(pCur);
			//opengl 4.2
			glDrawArraysInstancedBaseInstance(
				mpStateManager->GetPrimitiveTopology(),
				cmd->firstVertex,
				cmd->vertexCount,
				cmd->instanceCount,
				cmd->firstInstance);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndirect:
		{
			auto cmd = reinterpret_cast<const DrawIndirectInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			//opengl 4.3
			//uint32_t offset= static_cast<uint32_t>(cmd->offset);
			//TODO: offset
#if 1
			glMultiDrawArraysIndirect(
				mpStateManager->GetPrimitiveTopology(),
				nullptr,
				cmd->drawCount,
				cmd->stride);
#else
			for (uint32_t i = 0; i < cmd->drawCount; ++i)
			{
				//offset += i * sizeof(DrawIndirectCommand);
				glDrawArraysIndirect(
					mpStateManager->GetPrimitiveTopology(),
					(void *)&offset[i]);
			}

#endif
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndirectCount:
		{
			auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			mpStateManager->BindBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer *>(cmd->pCountBuffer)->GetHandle());
			//opengl 4.6
			glMultiDrawArraysIndirectCount(
				mpStateManager->GetPrimitiveTopology(),
				nullptr,
				static_cast<GLintptr>(cmd->countBufferOffset),
				cmd->maxDrawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexed:
		{
			auto cmd = reinterpret_cast<const DrawIndexedIndirectCommand *>(pCur);
			auto indexTypeSize=2;
			switch(mpStateManager->GetIndexType())
			{
			case GL_UNSIGNED_BYTE:
				indexTypeSize = 1;
				break;
			case GL_UNSIGNED_SHORT:
			default:
				indexTypeSize = 2;
				break;
			case GL_UNSIGNED_INT:
				indexTypeSize = 4;
				break;
			}
			auto offset = cmd->firstIndex*indexTypeSize;
			//opengl 4.2
			glDrawElementsInstancedBaseVertexBaseInstance(
				mpStateManager->GetPrimitiveTopology(),
				cmd->indexCount,
				Map(mCurIndexType),
				&offset,
				cmd->instanceCount,
				cmd->vertexOffset,
				cmd->firstInstance);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexedIndirect:
		{
			auto cmd = reinterpret_cast<const DrawIndirectInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			//opengl4.3
			glMultiDrawElementsIndirect(
				mpStateManager->GetPrimitiveTopology(),
				Map(mCurIndexType),
				nullptr,
				cmd->drawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexedIndirectCount:
		{
			auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			mpStateManager->BindBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer *>(cmd->pCountBuffer)->GetHandle());

			//opengl 4.6
			glMultiDrawElementsIndirectCount(
				mpStateManager->GetPrimitiveTopology(),
				Map(mCurIndexType),
				//&cmd->offset,
				nullptr,
				static_cast<GLintptr>(cmd->countBufferOffset),
				cmd->maxDrawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::EndRenderPass:
			mpCurFramebuffer->SetResolveFBOAttachment(mCurSubpass);
			mpCurFramebuffer->Resolve(Filter::LINEAR);
			mpCurFramebuffer = nullptr;
			mCurRenderPass = nullptr;
			mCurSubpass = 0;
			return 0;
		case GLCommandCode::SecondaryCommandBuffer:
		{
			auto cmd = reinterpret_cast<const ExecuteSecondaryCommandBufferInfo *>(pCur);
			for (uint32_t i = 0; i < cmd->count; ++i)
			{
				static_cast<GLCommandBuffer *>(&cmd->pCommandBuffers[i])->Execute();
			}
			return sizeof(*cmd);
		}
		case GLCommandCode::NextSubpass:
		{
			auto cmd = reinterpret_cast<const SubpassContents *>(pCur);
			mpCurFramebuffer->SetResolveFBOAttachment(mCurSubpass);
			mpCurFramebuffer->Resolve(Filter::LINEAR);
			++mCurSubpass;
			mpCurFramebuffer->SetRenderFBOAttachment(mCurSubpass);
			ClearBuffer();
			return sizeof(*cmd);
		}
		default:
			return 0;
		}
	}
	void GLCommandBuffer::Execute()
	{
		auto pCur = mBuffer.data();
		auto pEnd = pCur + mBuffer.size();
		GLCommandCode cmdCode;
		while (pCur < pEnd)
		{
			cmdCode = *(reinterpret_cast<GLCommandCode *>(pCur));
			pCur += sizeof(GLCommandCode);
			pCur += ExecuteCommand(cmdCode, pCur);
		}
	}
	template <class T>
	T *GLCommandBuffer::AllocateCommand(GLCommandCode commandCode, size_t realsize)
	{
		auto offset = mBuffer.size();
		if (realsize == 0)
			mBuffer.resize(offset + sizeof(GLCommandCode) + sizeof(T));
		else
			mBuffer.resize(offset + sizeof(GLCommandCode) + realsize);
		mBuffer[offset] = static_cast<uint8_t>(commandCode);
		return reinterpret_cast<T *>(&mBuffer[offset + sizeof(GLCommandCode)]);
	}
	template <>
	void *GLCommandBuffer::AllocateCommand<void>(GLCommandCode commandCode, size_t realsize)
	{
		auto offset = mBuffer.size();
		mBuffer.resize(offset + sizeof(GLCommandCode) + realsize);
		mBuffer[offset] = static_cast<uint8_t>(commandCode);
		return realsize == 0 ? &mBuffer[offset] : &mBuffer[offset + sizeof(GLCommandCode)];
	}
	void GLCommandBuffer::ExecuteSecondaryCommandBuffer(const ExecuteSecondaryCommandBufferInfo &secondaryCommandBufferInfo)
	{
		memcpy(AllocateCommand<void>(GLCommandCode::SecondaryCommandBuffer), &secondaryCommandBufferInfo, sizeof(ExecuteSecondaryCommandBufferInfo));
	}
	void GLCommandBuffer::Reset([[maybe_unused]] CommandBufferResetFlatBits flags)
	{
		mBuffer.clear();
	}
	void GLCommandBuffer::Begin([[maybe_unused]] const CommandBufferBeginInfo &beginInfo) {}
	void GLCommandBuffer::End() {}
	void GLCommandBuffer::BeginRenderPass(const RenderPassBeginInfo &beginInfo)
	{
		for (size_t i = 0; i < beginInfo.clearValueCount; ++i)
			static_cast<GLRenderPass *>(beginInfo.pRenderPass)->SetAttachmentClearValue(i, beginInfo.pClearValues[i]);
		memcpy(AllocateCommand<RenderPassBeginInfo>(GLCommandCode::BeginRenderPass), &beginInfo, sizeof(RenderPassBeginInfo));
	}
	void GLCommandBuffer::EndRenderPass()
	{
		AllocateCommand<void>(GLCommandCode::EndRenderPass);
	}
	void GLCommandBuffer::NextSubpass(SubpassContents subpassContents)
	{
		memcpy(AllocateCommand<SubpassContents>(GLCommandCode::NextSubpass), &subpassContents, sizeof(SubpassContents));
	}
	void GLCommandBuffer::BindPipeline(const BindPipelineInfo &info)
	{
		memcpy(AllocateCommand<BindPipelineInfo>(GLCommandCode::BindPipeline), &info, sizeof(BindPipelineInfo));
	}
	void GLCommandBuffer::CopyBuffer(const CopyBufferInfo &copyInfo)
	{
		auto size0 = sizeof(Buffer *) * 2 + sizeof(uint32_t);
		auto size1 = sizeof(BufferCopy) * copyInfo.regionCount;
		auto p = AllocateCommand<CopyBufferInfo>(GLCommandCode::CopyBuffer, size0 + size1);
		memcpy(p, &copyInfo, size0);
		memcpy(&p->pRegions, copyInfo.pRegions, size1);
	}
	void GLCommandBuffer::CopyImage(const CopyImageInfo &copyInfo)
	{
		memcpy(AllocateCommand<CopyImageInfo>(GLCommandCode::CopyImage), &copyInfo, sizeof(CopyImageInfo));
	}
	void GLCommandBuffer::CopyBufferToImage(const CopyBufferToImageInfo &copyInfo)
	{
		memcpy(AllocateCommand<CopyBufferToImageInfo>(GLCommandCode::CopyBufferToImage), &copyInfo, sizeof(CopyBufferToImageInfo));
	}
	void GLCommandBuffer::CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo)
	{
		memcpy(AllocateCommand<CopyImageToBufferInfo>(GLCommandCode::CopyImageToBuffer), &copyInfo, sizeof(CopyImageToBufferInfo));
	}
	void GLCommandBuffer::BlitImage(const BlitImageInfo &blitInfo)
	{
		memcpy(AllocateCommand<BlitImageInfo>(GLCommandCode::BlitImage), &blitInfo, sizeof(BlitImageInfo));
	}
	void GLCommandBuffer::BindVertexBuffer(const BindVertexBufferInfo &info)
	{
		auto p = AllocateCommand<BindVertexBufferInfo>(GLCommandCode::BindVertexBuffer, (sizeof(Buffer *) + sizeof(uint64_t)) * info.bindingCount + sizeof(uint32_t) * 2);

		size_t size = sizeof(BindVertexBufferInfo::bindingCount) + sizeof(BindVertexBufferInfo::firstBinding);
		memcpy(p, &info, size);
		size = info.bindingCount * sizeof(BindVertexBufferInfo::ppBuffers);
		memcpy(&p->ppBuffers, info.ppBuffers, size);
		size = info.bindingCount * sizeof(BindVertexBufferInfo::pOffsets);
		memcpy(&p->ppBuffers + info.bindingCount, info.pOffsets, size);
	}
	void GLCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info)
	{
		memcpy(AllocateCommand<BindIndexBufferInfo>(GLCommandCode::BindIndexBuffer), &info, sizeof(BindIndexBufferInfo));
		mCurIndexType = info.indexType;
		mCurIndexOffset = info.offset;
	}
	void GLCommandBuffer::BindDescriptorSets(const BindDescriptorSetsInfo &info)
	{
		auto p = AllocateCommand<BindDescriptorSetsInfo>(GLCommandCode::BindDescriptorSets,
														 sizeof(PipelineBindPoint) * 2 +
															 sizeof(PipelineLayout *) +
															 sizeof(uint32_t) * 3 +
															 sizeof(DescriptorSet *) * info.descriptorSetCount +
															 sizeof(uint32_t) * info.dynamicOffsetCount);
		size_t size = sizeof(PipelineBindPoint) * 2 + sizeof(PipelineLayout *) + sizeof(uint32_t) * 2;
		memcpy(p, &info, size);
		size = sizeof(DescriptorSet *) * info.descriptorSetCount;
		memcpy(&p->ppDescriptorSets, info.ppDescriptorSets, size);
		memcpy(&p->ppDescriptorSets + info.descriptorSetCount, &info.dynamicOffsetCount, sizeof(uint32_t));
		size = sizeof(uint32_t) * info.dynamicOffsetCount;
		auto p2 = reinterpret_cast<uint32_t *>(&p->ppDescriptorSets + info.descriptorSetCount) + 1;
		memcpy(p2, &info.pDynamicOffsets, sizeof(uint32_t) * info.dynamicOffsetCount);
	}
	void GLCommandBuffer::Draw(const DrawIndirectCommand &info)
	{
		memcpy(AllocateCommand<DrawIndirectCommand>(GLCommandCode::Draw), &info, sizeof(DrawIndirectCommand));
	}
	void GLCommandBuffer::DrawIndirect(const DrawIndirectInfo &info)
	{
		memcpy(AllocateCommand<DrawIndirectInfo>(GLCommandCode::DrawIndirect), &info, sizeof(DrawIndirectInfo));
	}
	void GLCommandBuffer::DrawIndirectCount(const DrawIndirectCountInfo &info)
	{
		memcpy(AllocateCommand<DrawIndirectCountInfo>(GLCommandCode::DrawIndirectCount), &info, sizeof(DrawIndirectCountInfo));
	}
	void GLCommandBuffer::DrawIndexed(const DrawIndexedIndirectCommand &info)
	{
		memcpy(AllocateCommand<DrawIndexedIndirectCommand>(GLCommandCode::DrawIndexed), &info, sizeof(DrawIndexedIndirectCommand));
	}
	void GLCommandBuffer::DrawIndexedIndirect(const DrawIndirectInfo &info)
	{
		if (mCurIndexOffset > 0)
		{
			//TODO: will repeat some times, how to fix?
			GLuint stageBuffer;
			glGenBuffers(1, &stageBuffer);
			mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, stageBuffer);
			glBufferStorage(GL_COPY_WRITE_BUFFER, sizeof(DrawIndexedIndirectCommand), nullptr, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);
			mpStateManager->BindBuffer(GL_COPY_READ_BUFFER, static_cast<GLBuffer *>(info.pBuffer)->GetHandle());
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(DrawIndexedIndirectCommand));

			auto p = (DrawIndexedIndirectCommand *)glMapBufferRange(GL_COPY_WRITE_BUFFER, 0, sizeof(DrawIndexedIndirectCommand),
																	GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT);

			p->firstIndex = static_cast<uint32_t>(mCurIndexOffset) / GetIndexTypeSize(mCurIndexType);
			glUnmapBuffer(GL_COPY_WRITE_BUFFER);

			mpStateManager->BindBuffer(GL_COPY_READ_BUFFER, stageBuffer);
			mpStateManager->BindBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLBuffer *>(info.pBuffer)->GetHandle());
			glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(DrawIndexedIndirectCommand));
			glDeleteBuffers(1, &stageBuffer);
			mpStateManager->NotifyReleaseBuffer(stageBuffer);
		}

		memcpy(AllocateCommand<DrawIndirectInfo>(GLCommandCode::DrawIndexedIndirect), &info, sizeof(DrawIndirectInfo));
	}
	void GLCommandBuffer::DrawIndexedIndirectCount(const DrawIndirectCountInfo &info)
	{
		memcpy(AllocateCommand<DrawIndirectCountInfo>(GLCommandCode::DrawIndexedIndirectCount), &info, sizeof(DrawIndirectCountInfo));
	}
	void GLCommandBuffer::PipeplineBarrier([[maybe_unused]] const PipelineBarrierInfo &info)
	{
		//TODO: opengl pipeline barrier
		LOG_VAR("PipeplineBarrier not implemented yet");
	}
	void GLCommandBuffer::PushConstants([[maybe_unused]] const PushConstantUpdateInfo &info)
	{
		LOG_VAR("PushConstants not implemented yet");
	}
} // namespace Shit
