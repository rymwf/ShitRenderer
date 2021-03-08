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
	class GLCommandBuffer final : public CommandBuffer
	{
		std::vector<uint8_t> mBuffer;
		GLStateManager* mStateManager;

	public:
		GLCommandBuffer(GLStateManager *stateManager, const CommandBufferCreateInfo &createInfo)
			: CommandBuffer(createInfo), mStateManager(stateManager) {}

		void CopyBuffer(const CopyBufferInfo &copyInfo) override;
		void CopyImage(const CopyImageInfo &copyInfo) override;
		void CopyBufferToImage(const CopyBufferToImageInfo &copyInfo) override;
		void CopyImageToBuffer(const CopyImageToBufferInfo &copyInfo) override;
		void BlitImage(const BlitImageInfo &blitInfo) override;
	};
} // namespace Shit
