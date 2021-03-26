/**
 * @file GLCommandBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandBuffer.h>
#include "GLPrerequisites.h"
namespace Shit
{
	enum class GLCommandCode : uint8_t
	{
		BeginRenderPass,
		BindIndexBuffer,
		BindPipeline,
		BindVertexBuffer,
		BindDescriptorSets,
		BlitImage,
		CopyBuffer,
		CopyBufferToImage,
		CopyImage,
		CopyImageToBuffer,
		Draw,
		DrawIndirect,
		DrawIndirectCount,
		DrawIndexed,
		DrawIndexedIndirect,
		DrawIndexedIndirectCount,
		EndRenderPass,
		SecondaryCommandBuffer,
		NextSubpass,
	};

	class GLCommandBuffer final : public CommandBuffer
	{
		std::vector<uint8_t> mBuffer;
		GLStateManager *mpStateManager;

		RenderPass *mCurRenderPass{};
		uint32_t mCurSubpass{};
		Pipeline *mCurPipeline{};
		IndexType mCurIndexType{};

		template <class T>
		T *AllocateCommand(GLCommandCode commandCode, size_t realSize= 0);

		size_t ExecuteCommand(GLCommandCode commandCode, const void *pCur);

		void ClearBuffer();

	public:
		GLCommandBuffer(GLStateManager *pStateManager, const CommandBufferCreateInfo &createInfo);

		void Execute();

		void Reset(CommandBufferResetFlatBits flags) override;
		void Begin(const CommandBufferBeginInfo &beginInfo) override;
		void End() override;

		void ExecuteSecondaryCommandBuffer(const ExecuteSecondaryCommandBufferInfo &secondaryCommandBufferInfo) override;
		void BeginRenderPass(const RenderPassBeginInfo &beginInfo) override;
		void EndRenderPass() override;
		void NextSubpass(SubpassContents subpassContents) override;
		void BindPipeline(const BindPipelineInfo &info) override;

		void CopyBuffer(const CopyBufferInfo &copyInfo) override;
		void CopyImage(const CopyImageInfo &copyInfo) override;
		void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
		void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
		void BlitImage(const BlitImageInfo &blitInfo) override;

		void BindVertexBuffer(const BindVertexBufferInfo &info) override;
		void BindIndexBuffer(const BindIndexBufferInfo &info) override;
		void BindDescriptorSets(const BindDescriptorSetsInfo &info) override;


		void Draw(const DrawIndirectCommand &info) override;
		void DrawIndirect(const DrawIndirectInfo &info) override;
		void DrawIndirectCount(const DrawIndirectCountInfo &info) override;
		void DrawIndexed(const DrawIndexedIndirectCommand &info) override;
		void DrawIndexedIndirect(const DrawIndirectInfo &info) override;
		void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) override;
	};
} // namespace Shit
