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
#include "GLDevice.h"

namespace Shit
{
	class GLBuffer final : public Buffer
	{
		GLuint mHandle;
		GLStateManager* mStateManager;

	public:
		GLBuffer(GLStateManager *stateManager, const BufferCreateInfo &createInfo, const void *pData)
			: Buffer(createInfo), mStateManager(stateManager)
		{
			glGenBuffers(1, &mHandle);
			// target do not matter when creating buffer
			glBindBuffer(GL_ARRAY_BUFFER, mHandle);

			GLbitfield flags{};
			if (static_cast<bool>(createInfo.flags & BufferCreateFlagBits::MUTABLE_FORMAT_BIT))
			{
				//glBufferData(GL_ARRAY_BUFFER, createInfo.size, pData, 0);
			}
			else
			{
				if (!(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::DEVICE_LOCAL_BIT))
					flags |= GL_CLIENT_STORAGE_BIT;
				if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
					flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
				if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
					flags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

				//(GLVersion.major * 10 + GLVersion.minor < 44 || GL_ARB_buffer_storage)
				glBufferStorage(GL_ARRAY_BUFFER, createInfo.size, pData, flags);
			}
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		constexpr GLuint GetHandle() const
		{
			return mHandle;
		}
		void MapBuffer(uint64_t offset, uint64_t size, void **ppData) override
		{
			//glMapBufferRange(GL_ARRAY_BUFFER,);
		}
		void UnMapBuffer() override
		{
		}
	};
} // namespace Shit
