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

#include "GLState.hpp"
namespace Shit {
GLBuffer::GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, const void *pData)
    : Buffer(createInfo), mpStateManager(pStateManager) {
    CreateBuffer(pData);
}
GLBuffer::GLBuffer(GLStateManager *pStateManager, const BufferCreateInfo &createInfo, int val)
    : Buffer(createInfo), mpStateManager(pStateManager) {
    CreateBuffer(val);
}
void GLBuffer::CreateBuffer(void const *pData) {
    glGenBuffers(1, &mHandle);
    // target do not matter when creating buffer
    mpStateManager->PushBuffer(GL_ARRAY_BUFFER, mHandle);

    GLbitfield flags{};
    if (static_cast<bool>(mCreateInfo.flags & BufferCreateFlagBits::MUTABLE_FORMAT_BIT)) {
        // glBufferData(GL_ARRAY_BUFFER, createInfo.size, pData, 0);
        ST_LOG("mutable format buffer is not implemented yet")
    } else {
        // if (!(createInfo.memoryPropertyFlags &
        // MemoryPropertyFlagBits::DEVICE_LOCAL_BIT))
        //	flags |= GL_CLIENT_STORAGE_BIT;
        if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT))
            flags |= GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
            flags |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;

        //(g_GLVersion.major * 10 + g_GLVersion.minor < 44 || GL_ARB_buffer_storage)
        glBufferStorage(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mCreateInfo.size), pData, flags);
        // glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(createInfo.size),
        // pData, GL_DYNAMIC_DRAW);
    }
    mpStateManager->PopBuffer();
}
void GLBuffer::CreateBuffer(int val) {
    std::vector<int> data((std::max)(mCreateInfo.size / 4, 1ull), val);
    CreateBuffer(data.data());
}
GLBuffer::~GLBuffer() {
    mpStateManager->NotifyReleaseBuffer(mHandle);
    glDeleteBuffers(1, &mHandle);
}

void GLBuffer::Resize(uint64_t size) {
    mpStateManager->NotifyReleaseBuffer(mHandle);
    glDeleteBuffers(1, &mHandle);
    mMemorySize = mCreateInfo.size = size;
    CreateBuffer(nullptr);
}
void GLBuffer::MapMemory(uint64_t offset, uint64_t size, void **ppData) const {
    GLbitfield access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_COHERENT_BIT))
        access |= GL_MAP_COHERENT_BIT | GL_MAP_PERSISTENT_BIT;
    else
        access |= GL_MAP_FLUSH_EXPLICIT_BIT;  //|GL_MAP_UNSYNCHRONIZED_BIT cannot
                                              // use with read bit
    mpStateManager->BindBuffer(GL_ARRAY_BUFFER, mHandle);
    *ppData = glMapBufferRange(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size), access);
    //*ppData = glMapNamedBufferRange(mHandle, static_cast<GLintptr>(offset),
    // static_cast<GLsizeiptr>(size), access);
}
void GLBuffer::UnMapMemory() const {
    mpStateManager->BindBuffer(GL_ARRAY_BUFFER, mHandle);
    glUnmapNamedBuffer(mHandle);
    // glUnmapBuffer(GL_ARRAY_BUFFER);
    // mpStateManager->PopBuffer();
}
void GLBuffer::FlushMappedMemoryRange(uint64_t offset, uint64_t size) const {
    mpStateManager->BindBuffer(GL_ARRAY_BUFFER, mHandle);
    glFlushMappedBufferRange(GL_ARRAY_BUFFER, static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    // mpStateManager->PopBuffer();
    // glFlushMappedNamedBufferRange(mHandle, static_cast<GLintptr>(offset),
    // static_cast<GLsizeiptr>(size));
}
void GLBuffer::InvalidateMappedMemoryRange(ST_MAYBE_UNUSED uint64_t offset, ST_MAYBE_UNUSED uint64_t size) const {
    ST_LOG("error, GLBuffer InvalidateMappedMemoryRange not implemented yet");
    // glInvalidateBufferSubData(mHandle, static_cast<GLintptr>(offset),
    // static_cast<GLsizeiptr>(size));
}
void GLBuffer::UpdateSubData(uint64_t offset, uint64_t size, void const *pData) const {
    mpStateManager->PushBuffer(GL_ARRAY_BUFFER, mHandle);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, pData);
    mpStateManager->PopBuffer();
}
void GLBuffer::UpdateSubData(uint64_t offset, uint64_t size, int val) const {
    std::vector<int> data((std::max)(size, 1ull), val);
    mpStateManager->PushBuffer(GL_ARRAY_BUFFER, mHandle);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data.data());
    mpStateManager->PopBuffer();
}
}  // namespace Shit