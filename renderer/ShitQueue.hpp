/**
 * @file ShitQueue.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include "ShitCommandBuffer.hpp"
#include "ShitRendererPrerequisites.hpp"
namespace Shit {
class Queue {
protected:
    uint32_t _familyIndex;
    uint32_t _index;
    float _priority;

public:
    Queue(uint32_t familyIndex, uint32_t index, float priority)
        : _familyIndex(familyIndex), _index(index), _priority(priority) {}

    virtual ~Queue() {}
    virtual void Submit(std::span<const SubmitInfo> submitInfos, Fence *fence) = 0;
    virtual Result Present(const PresentInfo &presentInfo) = 0;
    virtual Result Present(const PresentInfo2 &presentInfo) = 0;
    virtual void WaitIdle() = 0;
    constexpr uint32_t GetFamilyIndex() const { return _familyIndex; }
    constexpr uint32_t GetIndex() const { return _index; }
    constexpr float GetPriority() const { return _priority; }
};
}  // namespace Shit
