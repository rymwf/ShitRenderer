/**
 * @file ShitMemory.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once

#include "ShitRendererPrerequisites.hpp"
namespace Shit {
class Memory {
protected:
    MemoryAllocateInfo mAllocateInfo;

public:
    Memory(const MemoryAllocateInfo &allocateInfo) : mAllocateInfo(allocateInfo) {}
    virtual ~Memory() {}
};
}  // namespace Shit
