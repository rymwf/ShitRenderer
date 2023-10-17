/**
 * @file GLQueue.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "GLQueue.hpp"

#include "GLCommandBuffer.hpp"
#include "GLFence.hpp"
#include "GLFramebuffer.hpp"
#include "GLRenderSystem.hpp"
#include "GLSemaphore.hpp"
#include "GLState.hpp"
#include "GLSurface.hpp"
#include "GLSwapchain.hpp"

namespace Shit {
void GLQueue::Submit(std::span<const SubmitInfo> submitInfos, Fence *pFence) {
    g_RenderSystem->GetMainSurface()->MakeCurrent();
    uint32_t i;
    for (auto &&submitInfo : submitInfos) {
        for (i = 0; i < submitInfo.waitSemaphoreCount; ++i) {
            static_cast<const GLSemaphore *>(submitInfo.pWaitSemaphores[i])->Wait();
        }
        for (i = 0; i < submitInfo.commandBufferCount; ++i) {
            static_cast<GLCommandBuffer *>(const_cast<CommandBuffer *>(submitInfo.pCommandBuffers[i]))->Execute();
        }
        for (i = 0; i < submitInfo.signalSemaphoreCount; ++i) {
            static_cast<const GLSemaphore *>(submitInfo.pSignalSemaphores[i])->Insert();
        }
        if (pFence) static_cast<GLFence *>(pFence)->Insert();
    }
}
Result GLQueue::Present(const PresentInfo &presentInfo) {
    uint32_t i;
    for (i = 0; i < presentInfo.waitSemaphoreCount; ++i) {
        static_cast<const GLSemaphore *>(presentInfo.pWaitSemaphores[i])->Wait();
    }
    for (i = 0; i < presentInfo.swapchainCount; ++i) {
        static_cast<const GLSwapchain *>(presentInfo.pSwapchains[i])->Present();
    }
    return Result::SUCCESS;
}
Result GLQueue::Present(const PresentInfo2 &presentInfo) {
    for (auto e : presentInfo.waitSemaphores) {
        static_cast<const GLSemaphore *>(e)->Wait();
    }
    for (auto e : presentInfo.swapchains) {
        static_cast<const GLSwapchain *>(e)->Present();
    }
    return Result::SUCCESS;
}
}  // namespace Shit
