/**
 * @file VKBuffer.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKBuffer.hpp"

#include "VKCommandPool.hpp"
#include "VKDevice.hpp"
#include "VKQueue.hpp"

namespace Shit {
VKBuffer::VKBuffer(VKDevice *pDevice, VkPhysicalDevice physicalDevice, const BufferCreateInfo &createInfo)
    : Buffer(createInfo), VKDeviceObject(pDevice), mPhysicalDevice(physicalDevice) {
    VkBufferCreateInfo info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        createInfo.size,
        Map(createInfo.usage),
        VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create vertex buffer");

    VkMemoryRequirements bufferMemoryRequirements;
    vkGetBufferMemoryRequirements(mpDevice->GetHandle(), mHandle, &bufferMemoryRequirements);

    // ST_LOG_VAR(bufferMemoryRequirements.size);
    // ST_LOG_VAR(bufferMemoryRequirements.alignment);
    // ST_LOG_VAR(bufferMemoryRequirements.memoryTypeBits); //typebits is the
    // memorytype indices in physical memory properties

    auto memoryTypeIndex = VK::findMemoryTypeIndex(physicalDevice, bufferMemoryRequirements.memoryTypeBits,
                                                   Map(createInfo.memoryPropertyFlags));  // the index of memory type

    mMemory = VK::allocateMemory(mpDevice->GetHandle(), bufferMemoryRequirements.size, memoryTypeIndex);

    // update createInfo buffer size
    mMemorySize = bufferMemoryRequirements.size;

    vkBindBufferMemory(mpDevice->GetHandle(), mHandle, mMemory, 0);
}
void VKBuffer::Resize(uint64_t size) {
    if (size == mCreateInfo.size) return;
    vkDestroyBuffer(mpDevice->GetHandle(), mHandle, nullptr);
    mCreateInfo.size = size;
    VkBufferCreateInfo info{
        VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        nullptr,
        0,
        mCreateInfo.size,
        Map(mCreateInfo.usage),
        VK_SHARING_MODE_EXCLUSIVE,
    };
    if (vkCreateBuffer(mpDevice->GetHandle(), &info, nullptr, &mHandle) != VK_SUCCESS)
        ST_THROW("failed to create vertex buffer");

    if (size > mMemorySize) {
        vkFreeMemory(mpDevice->GetHandle(), mMemory, nullptr);

        VkMemoryRequirements bufferMemoryRequirements;
        vkGetBufferMemoryRequirements(mpDevice->GetHandle(), mHandle, &bufferMemoryRequirements);

        // ST_LOG_VAR(bufferMemoryRequirements.size);
        // ST_LOG_VAR(bufferMemoryRequirements.alignment);
        // ST_LOG_VAR(bufferMemoryRequirements.memoryTypeBits); //typebits is the
        // memorytype indices in physical memory properties

        auto memoryTypeIndex =
            VK::findMemoryTypeIndex(mPhysicalDevice, bufferMemoryRequirements.memoryTypeBits,
                                    Map(mCreateInfo.memoryPropertyFlags));  // the index of memory type

        mMemory = VK::allocateMemory(mpDevice->GetHandle(), bufferMemoryRequirements.size, memoryTypeIndex);

        // update createInfo buffer size
        mMemorySize = bufferMemoryRequirements.size;
    }
    vkBindBufferMemory(mpDevice->GetHandle(), mHandle, mMemory, 0);
}
void VKBuffer::MapMemory(uint64_t offset, uint64_t size, void **ppData) const {
    // this function will copy buffer before returns,
    // which is unnecessary when we do not need the data
    vkMapMemory(mpDevice->GetHandle(), mMemory, offset, size, 0, ppData);
}
void VKBuffer::UnMapMemory() const { vkUnmapMemory(mpDevice->GetHandle(), mMemory); }
void VKBuffer::FlushMappedMemoryRange(uint64_t offset, uint64_t size) const {
    auto range = VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, mMemory, offset, size};
    vkFlushMappedMemoryRanges(mpDevice->GetHandle(), 1, &range);
}
void VKBuffer::InvalidateMappedMemoryRange(uint64_t offset, uint64_t size) const {
    auto range = VkMappedMemoryRange{VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE, nullptr, mMemory, offset, size};
    vkInvalidateMappedMemoryRanges(mpDevice->GetHandle(), 1, &range);
}
void VKBuffer::UpdateSubData(uint64_t offset, uint64_t size, void const *pData) const {
    if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT)) {
        void *data;
        MapMemory(offset, size, &data);
        memcpy(data, pData, static_cast<size_t>(mCreateInfo.size));
        UnMapMemory();
    } else if (static_cast<bool>(mCreateInfo.usage & BufferUsageFlagBits::TRANSFER_DST_BIT)) {
        BufferCreateInfo stagingBufferCreateInfo{
            {},
            mCreateInfo.size,
            BufferUsageFlagBits::TRANSFER_SRC_BIT,
            MemoryPropertyFlagBits::HOST_VISIBLE_BIT | MemoryPropertyFlagBits::HOST_COHERENT_BIT,
        };
        VKBuffer stagingbuffer{mpDevice, mPhysicalDevice, stagingBufferCreateInfo};
        void *data;
        stagingbuffer.MapMemory(offset, size, &data);
        memcpy(data, pData, static_cast<size_t>(mCreateInfo.size));
        stagingbuffer.UnMapMemory();

        static_cast<VKDevice *>(mpDevice)->ExecuteOneTimeCommand([&](CommandBuffer *cmdBuffer) {
            BufferCopy region{offset, offset, size};
            cmdBuffer->CopyBuffer({&stagingbuffer, this, 1, &region});
        });
    } else {
        ST_THROW(
            "buffer should be created with a BufferUsageFlagBits::TRANSFER_DST_BIT "
            "or MemoryPropertyFlagBits::HOST_VISIBLE_BIT");
    }
}
void VKBuffer::UpdateSubData(uint64_t offset, uint64_t size, int val) const {
    if (static_cast<bool>(mCreateInfo.memoryPropertyFlags & MemoryPropertyFlagBits::HOST_VISIBLE_BIT)) {
        void *data;
        MapMemory(offset, size, &data);
        memset(data, val, static_cast<size_t>(mCreateInfo.size));
        UnMapMemory();
    } else if (static_cast<bool>(mCreateInfo.usage & BufferUsageFlagBits::TRANSFER_DST_BIT)) {
        static_cast<VKDevice *>(mpDevice)->ExecuteOneTimeCommand([&](CommandBuffer *cmdBuffer) {
            cmdBuffer->FillBuffer({this, offset, size, (uint32_t)val});
        });
    } else {
        ST_THROW(
            "buffer should be created with a BufferUsageFlagBits::TRANSFER_DST_BIT "
            "or MemoryPropertyFlagBits::HOST_VISIBLE_BIT");
    }
}
}  // namespace Shit
