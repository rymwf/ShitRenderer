/**
 * @file ShitCommandBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
namespace Shit
{
	class CommandBuffer
	{
	protected:
		CommandBufferCreateInfo mCreateInfo;

		CommandBuffer(const CommandBufferCreateInfo &createInfo) : mCreateInfo(createInfo) {}

	public:
		virtual ~CommandBuffer() {}

		constexpr const CommandBufferCreateInfo *GetCreateInfoPtr() const
		{
			return &mCreateInfo;
		}

		virtual void Reset(CommandBufferResetFlatBits flags) {}
		virtual void Begin(const CommandBufferBeginInfo &beginInfo) {}
		virtual void End() {}
		virtual void ExecuteSecondaryCommandBuffer(const std::vector<CommandBuffer *> &secondaryCommandBuffers) {}
		virtual void BeginRenderPass(const RenderPassBeginInfo &beginInfo, const SubpassBeginInfo &subpassBeginInfo) {}
		virtual void EndRenderPass() {}
		virtual void NextSubpass(const SubpassBeginInfo &subpassBeginInfo) {}
		virtual void BindPipeline(PipelineBindPoint bindPoint,Pipeline* pPipeline){}

		virtual void CopyBuffer(const CopyBufferInfo &copyInfo) = 0;
		virtual void CopyImage(const CopyImageInfo &copyInfo) = 0;
		virtual void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) = 0;
		virtual void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) = 0;
		virtual void BlitImage(const BlitImageInfo &blitInfo) = 0;

		virtual void BindVertexBuffer(const BindVertexBufferInfo& info){};
		virtual void BindIndexBuffer(const BindIndexBufferInfo& info){}

		virtual void Draw(const DrawIndirectCommand& info){};
		virtual void DrawIndirect(const DrawIndirectInfo &info) {}
		virtual void DrawIndirectCount(const DrawIndirectCountInfo &info) {}
		virtual void DrawIndexed(const DrawIndexedIndirectCommand &info) {}
		virtual void DrawIndexedIndirect(const DrawIndirectInfo &info){}
		virtual void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) {}
	};
}