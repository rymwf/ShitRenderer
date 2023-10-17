/**
 * @file GLQueue.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitQueue.hpp>

#include "GLPrerequisites.hpp"

// TODO: make GLQueue a thread
namespace Shit {
class GLQueue final : public Queue {
    GLStateManager *mpStateManager;
    GLDevice *mpDevice;

public:
    GLQueue(GLDevice *pDevice, GLStateManager *pStateManager, uint32_t familyIndex, uint32_t index, float priority)
        : Queue(familyIndex, index, priority), mpStateManager(pStateManager), mpDevice(pDevice) {}

    void Submit(std::span<const SubmitInfo> submitInfos, Fence *pFence) override;
    Result Present(const PresentInfo &presentInfo) override;
    Result Present(const PresentInfo2 &presentInfo) override;
    void WaitIdle() override { glFinish(); }
};
}  // namespace Shit
