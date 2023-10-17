#include "VKMemory.hpp"
namespace Shit {
VKMemory::VKMemory(VKDevice *device, const MemoryAllocateInfo &allocateInfo)
    : VKDeviceObject(device), Memory(allocateInfo) {
    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, nullptr, mAllocateInfo.size,
                                   mAllocateInfo.memoryTypeIndex};

    CHECK_VK_RESULT(vkAllocateMemory(mpDevice->GetHandle(), &allocInfo, nullptr, &mHandle))
}
VKMemory::~VKMemory() {}
}  // namespace Shit