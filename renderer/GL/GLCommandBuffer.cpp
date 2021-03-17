/**
 * @file GLCommandBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLCommandBuffer.h"
#include "GLFramebuffer.h"
#include "GLRenderPass.h"
#include "GLPipeline.h"
#include "GLBuffer.h"
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
			auto framebuffer = static_cast<GLFramebuffer *>(cmd->pFramebuffer)->GetHandle();
			mpStateManager->BindDrawFramebuffer(framebuffer);
			mCurRenderPass = cmd->pRenderPass;
			mCurSubpass = 0;
			//TODO: what is renderArea
			//clear value
			ClearBuffer();
			return sizeof(*cmd);
		}
		case GLCommandCode::BindIndexBuffer:
		{
			auto cmd = reinterpret_cast<const BindIndexBufferInfo *>(pCur);
			mpStateManager->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			mCurIndexType = cmd->indexType;
			mCurIndexOffset = cmd->offset;
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
					mpStateManager->BindVertexArray(static_cast<GLGraphicsPipeline *>(pPipeline)->GetVertexArray());
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
			auto graphicsPipeline = dynamic_cast<GLGraphicsPipeline *>(mCurPipeline);
			auto &&vertexInputState = graphicsPipeline->GetCreateInfoPtr()->vertexInputState;
			for (auto &&attrib : vertexInputState.vertexAttributeDescriptions)
			{
				auto index = attrib.binding + cmd->firstBinding;
				mpStateManager->BindBuffer(GL_ARRAY_BUFFER, static_cast<GLBuffer *>(&cmd->pBuffers[index])->GetHandle());
				uint32_t offset = attrib.offset + cmd->pOffsets[index];
				glVertexAttribPointer(
					attrib.location,
					attrib.components,
					Map(attrib.dataType),
					attrib.normalized,
					vertexInputState.vertexBindingDescriptions[attrib.binding].stride,
					&offset);
				glVertexAttribDivisor(attrib.location, vertexInputState.vertexBindingDescriptions[attrib.binding].divisor);
				glEnableVertexAttribArray(attrib.location);
			}
			return sizeof(*cmd);
		}
		case GLCommandCode::BlitImage:
		{
			auto cmd = reinterpret_cast<const BlitImageInfo *>(pCur);
			return sizeof(*cmd);
		}
		case GLCommandCode::CopyBuffer:
		{
			auto cmd = reinterpret_cast<const CopyBufferInfo *>(pCur);
			return sizeof(*cmd);
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
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
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
			uint32_t offset = cmd->offset;
			glMultiDrawArraysIndirect(
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
				&offset,
				cmd->drawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndirectCount:
		{
			auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			mpStateManager->BindBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer *>(cmd->pCountBuffer)->GetHandle());
			//opengl 4.6
			uint32_t offset = cmd->offset;
			glMultiDrawArraysIndirectCount(
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
				&offset,
				static_cast<GLintptr>(cmd->countBufferOffset),
				cmd->maxDrawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexed:
		{
			auto cmd = reinterpret_cast<const DrawIndexedIndirectCommand *>(pCur);
			uint32_t firstIndex = cmd->firstIndex + mCurIndexOffset * static_cast<uint32_t>(IndexType::UINT8) / static_cast<uint32_t>(mCurIndexType);
			//opengl 4.2
			glDrawElementsInstancedBaseVertexBaseInstance(
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
				cmd->indexCount,
				Map(mCurIndexType),
				&firstIndex,
				cmd->instanceCount,
				cmd->vertexOffset,
				cmd->firstInstance);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexedIndirect:
		{
			auto cmd = reinterpret_cast<const DrawIndirectInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			uint32_t offset = cmd->offset + mCurIndexOffset * static_cast<uint32_t>(IndexType::UINT8) / static_cast<uint32_t>(mCurIndexType);
			//opengl4.3
			glMultiDrawElementsIndirect(
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
				Map(mCurIndexType),
				&offset,
				cmd->drawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::DrawIndexedIndirectCount:
		{
			auto cmd = reinterpret_cast<const DrawIndirectCountInfo *>(pCur);
			mpStateManager->BindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLBuffer *>(cmd->pBuffer)->GetHandle());
			mpStateManager->BindBuffer(GL_PARAMETER_BUFFER, static_cast<GLBuffer *>(cmd->pCountBuffer)->GetHandle());
			uint32_t offset = cmd->offset + mCurIndexOffset;
			//opengl 4.6
			glMultiDrawElementsIndirectCount(
				Map(dynamic_cast<GLGraphicsPipeline *>(mCurPipeline)->GetCreateInfoPtr()->inputAssemblyState.topology),
				Map(mCurIndexType),
				&offset,
				static_cast<GLintptr>(cmd->countBufferOffset),
				cmd->maxDrawCount,
				cmd->stride);
			return sizeof(*cmd);
		}
		case GLCommandCode::EndRenderPass:
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
			++mCurSubpass;
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
	T *GLCommandBuffer::AllocateCommand(GLCommandCode commandCode, size_t extraSize)
	{
		auto offset = mBuffer.size();
		mBuffer.resize(offset + sizeof(GLCommandCode) + sizeof(T) + extraSize);
		mBuffer[offset] = static_cast<uint8_t>(commandCode);
		return reinterpret_cast<T *>(&mBuffer[offset + sizeof(GLCommandCode)]);
	}
	template <>
	void *GLCommandBuffer::AllocateCommand<void>(GLCommandCode commandCode, size_t extraSize)
	{
		auto offset = mBuffer.size();
		mBuffer.resize(offset + sizeof(GLCommandCode) + extraSize);
		mBuffer[offset] = static_cast<uint8_t>(commandCode);
		return reinterpret_cast<void *>(&mBuffer[offset + sizeof(GLCommandCode)]);
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
		memcpy(AllocateCommand<CopyBufferInfo>(GLCommandCode::CopyBuffer), &copyInfo, sizeof(CopyBufferInfo));
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
		memcpy(AllocateCommand<BindVertexBufferInfo>(GLCommandCode::BindVertexBuffer), &info, sizeof(BindVertexBufferInfo));
	}
	void GLCommandBuffer::BindIndexBuffer(const BindIndexBufferInfo &info)
	{
		memcpy(AllocateCommand<BindIndexBufferInfo>(GLCommandCode::BindIndexBuffer), &info, sizeof(BindIndexBufferInfo));
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
		memcpy(AllocateCommand<DrawIndirectInfo>(GLCommandCode::DrawIndexedIndirect), &info, sizeof(DrawIndirectInfo));
	}
	void GLCommandBuffer::DrawIndexedIndirectCount(const DrawIndirectCountInfo &info)
	{
		memcpy(AllocateCommand<DrawIndirectCountInfo>(GLCommandCode::DrawIndexedIndirectCount), &info, sizeof(DrawIndirectCountInfo));
	}

} // namespace Shit
