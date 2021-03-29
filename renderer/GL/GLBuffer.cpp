/**
 * @file GLBuffer.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "GLBuffer.hpp"
namespace Shit
{

	GLBuffer::GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, const void *pData)
		: Buffer(createInfo), mpStateManager(pStateManager)
	{
		glGenBuffers(1, &mHandle);
		// target do not matter when creating buffer
		mpStateManager->PushBuffer(GL_ARRAY_BUFFER, mHandle);

		GLbitfield flags{};
		if (static_cast<bool>(createInfo.flags & BufferCreateFlagBits::MUTABLE_FORMAT_BIT))
		{
			//glBufferData(GL_ARRAY_BUFFER, createInfo.size, pData, 0);
		}
		else
		{
			//if (!(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::DEVICE_LOCAL_BIT))
			//	flags |= GL_CLIENT_STORAGE_BIT;
			if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
				flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
			if (static_cast<bool>(createInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
				flags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

			//(GLVersion.major * 10 + GLVersion.minor < 44 || GL_ARB_buffer_storage)
			glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(createInfo.size), pData, flags);
			//glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(createInfo.size), pData, GL_DYNAMIC_DRAW);
		}
		mpStateManager->PopBuffer();
	}
	void GLBuffer::MapBuffer(uint64_t offset, uint64_t size, void **ppData)
	{
		//mpStateManager->PushBuffer(GL_ARRAY_BUFFER, mHandle);
		GLbitfield access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
			access |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
		else
			access |= GL_MAP_FLUSH_EXPLICIT_BIT;
		//*ppData = glMapBufferRange(GL_ARRAY_BUFFER, offset, size, access);
		*ppData = glMapNamedBufferRange(mHandle, offset, size, access);
	}
	void GLBuffer::UnMapBuffer()
	{
		glUnmapNamedBuffer(mHandle);
		//glUnmapBuffer(GL_ARRAY_BUFFER);
		//mpStateManager->PopBuffer();
	}
}