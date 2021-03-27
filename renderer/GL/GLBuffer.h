/**
 * @file GLBuffer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitBuffer.h>
#include "GLPrerequisites.h"

namespace Shit
{
	class GLBuffer final : public Buffer
	{
		GLuint mHandle;
		GLStateManager *mpStateManager;
		bool mIsBound{};

	public:
		GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, const void *pData);
		~GLBuffer() override
		{
			mpStateManager->NotifyReleaseBuffer(mHandle);
			glDeleteBuffers(1, &mHandle);
		}
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
		void MapBuffer(uint64_t offset, uint64_t size, void **ppData) override;
		void UnMapBuffer() override;
	};
} // namespace Shit
