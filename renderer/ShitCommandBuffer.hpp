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
#include "ShitRendererPrerequisites.hpp"
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

		virtual void ExecuteSecondaryCommandBuffer(const ExecuteSecondaryCommandBufferInfo &secondaryCommandBufferInfo) = 0;

		virtual void Reset(CommandBufferResetFlatBits flags) = 0;
		virtual void Begin(const CommandBufferBeginInfo &beginInfo) = 0;
		virtual void End() = 0;
		virtual void BeginRenderPass(const RenderPassBeginInfo &beginInfo) = 0;
		virtual void EndRenderPass() = 0;
		virtual void NextSubpass(SubpassContents subpassContents) = 0;
		virtual void BindPipeline(const BindPipelineInfo &info) = 0;

		virtual void CopyBuffer(const CopyBufferInfo &copyInfo) = 0;
		virtual void CopyImage(const CopyImageInfo &copyInfo) = 0;
		virtual void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) = 0;
		virtual void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) = 0;
		virtual void BlitImage(const BlitImageInfo &blitInfo) = 0;

		virtual void BindVertexBuffer(const BindVertexBufferInfo &info) = 0;
		virtual void BindIndexBuffer(const BindIndexBufferInfo &info) = 0;
		virtual void BindDescriptorSets(const BindDescriptorSetsInfo &info) = 0;

		virtual void Draw(const DrawIndirectCommand &info) = 0;
		virtual void DrawIndirect(const DrawIndirectInfo &info) = 0;
		virtual void DrawIndirectCount(const DrawIndirectCountInfo &info) = 0;
		virtual void DrawIndexed(const DrawIndexedIndirectCommand &info) = 0;
		virtual void DrawIndexedIndirect(const DrawIndirectInfo &info) = 0;
		virtual void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) = 0;

		virtual void PipeplineBarrier(const PipelineBarrierInfo& info)=0;
	};
}