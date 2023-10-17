/**
 * @file ShitFence.hpp
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
class Fence {
protected:
    FenceCreateInfo mCreateInfo;
    Fence(const FenceCreateInfo &createInfo) : mCreateInfo(createInfo) {}

public:
    virtual ~Fence() {}
    constexpr const FenceCreateInfo *GetCreateInfoPtr() const { return &mCreateInfo; }
    virtual void Reset() const = 0;
    virtual Result WaitFor(uint64_t timeout) const = 0;
};
}  // namespace Shit
