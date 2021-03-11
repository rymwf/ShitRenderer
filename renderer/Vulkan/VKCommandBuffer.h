/**
 * @file VKCommandBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitCommandBuffer.h>
#include "VKPrerequisites.h"
namespace Shit
{

	class VKCommandBuffer final : public CommandBuffer
	{
		VkCommandBuffer mHandle;
		VkDevice mDevice;
		VkCommandPool mCommandPool;

	public:
		VKCommandBuffer(VkDevice device, VkCommandPool commandPool, const CommandBufferCreateInfo &createInfo);
		constexpr VkCommandBuffer GetHandle()
		{
			return mHandle;
		}
		~VKCommandBuffer() override
		{
		}
		void Destroy()
		{
			vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mHandle);
		}
		void Reset(CommandBufferResetFlatBits flags) override;
		void Begin(const CommandBufferBeginInfo &beginInfo) override;
		void End() override;
		void ExecuteSecondaryCommandBuffer(const std::vector<CommandBuffer *> &secondaryCommandBuffers) override;
		void BeginRenderPass(const RenderPassBeginInfo &beginInfo, const SubpassBeginInfo &subpassBeginInfo) override;
		void EndRenderPass() override;
		void NextSubpass(const SubpassBeginInfo &subpassBeginInfo) override;
		void BindPipeline(PipelineBindPoint bindPoint, Pipeline *pPipeline) override;

		void CopyBuffer(const CopyBufferInfo &copyInfo) override;
		void CopyImage(const CopyImageInfo &copyInfo) override;
		void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
		void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
		void BlitImage(const BlitImageInfo &blitInfo) override;

		void BindVertexBuffer(const BindVertexBufferInfo& info)override;
		void BindIndexBuffer(const BindIndexBufferInfo& info)override;

		void Draw(const DrawIndirectCommand &info) override;
		void DrawIndirect(const DrawIndirectInfo &info) override;
		void DrawIndirectCount(const DrawIndirectCountInfo &info) override;
		void DrawIndexed(const DrawIndexedIndirectCommand &info) override;
		void DrawIndexedIndirect(const DrawIndirectInfo &info) override;
		void DrawIndexedIndirectCount(const DrawIndirectCountInfo &info) override;
	};
}
