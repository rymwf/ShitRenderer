/**
 * @file VKQueue.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "VKQueue.hpp"

#include "VKCommandBuffer.hpp"
#include "VKFence.hpp"
#include "VKSemaphore.hpp"
#include "VKSwapchain.hpp"

namespace Shit {
void VKQueue::Submit(std::span<const SubmitInfo> submitInfos, Fence *pFence) {
    static auto v_semaphore =
        std::views::transform([](const Semaphore *p) { return static_cast<const VKSemaphore *>(p)->GetHandle(); });
    static auto v_commandBuffer = std::views::transform(
        [](const CommandBuffer *p) { return static_cast<const VKCommandBuffer *>(p)->GetHandle(); });

    struct SubmitInfoCache {
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitDstStageMask;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VkCommandBuffer> commandBuffers;
    };
    static auto v_s = std::views::transform([](const SubmitInfo &e) -> SubmitInfoCache {
        SubmitInfoCache ret;
        std::ranges::copy(std::span(e.pWaitSemaphores, e.waitSemaphoreCount) | v_semaphore,
                          std::back_inserter(ret.waitSemaphores));
        std::ranges::copy(std::span(e.pSignalSemaphores, e.signalSemaphoreCount) | v_semaphore,
                          std::back_inserter(ret.signalSemaphores));
        std::ranges::copy(std::span(e.pCommandBuffers, e.commandBufferCount) | v_commandBuffer,
                          std::back_inserter(ret.commandBuffers));
        ret.waitDstStageMask.resize(e.waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
        return ret;
    });

    static auto v_final = std::views::transform([](SubmitInfoCache &e) {
        return VkSubmitInfo{
            VK_STRUCTURE_TYPE_SUBMIT_INFO,
            nullptr,
            static_cast<uint32_t>(e.waitSemaphores.size()),
            e.waitSemaphores.data(),
            e.waitDstStageMask.data(),
            static_cast<uint32_t>(e.commandBuffers.size()),
            e.commandBuffers.data(),
            static_cast<uint32_t>(e.signalSemaphores.size()),
            e.signalSemaphores.data(),
        };
    });

    std::vector<SubmitInfoCache> submitInfoCache;
    std::ranges::copy(submitInfos | v_s, std::back_inserter(submitInfoCache));

    std::vector<VkSubmitInfo> infos;
    std::ranges::copy(submitInfoCache | v_final, std::back_inserter(infos));

    CHECK_VK_RESULT(vkQueueSubmit(mHandle, static_cast<uint32_t>(infos.size()), infos.data(),
                                  pFence ? static_cast<VKFence *>(pFence)->GetHandle() : VK_NULL_HANDLE))
}
Result VKQueue::Present(const PresentInfo &presentInfo) {
    static auto v_semaphore =
        std::views::transform([](const Semaphore *p) { return static_cast<const VKSemaphore *>(p)->GetHandle(); });
    static auto v_swapchain =
        std::views::transform([](const Swapchain *p) { return static_cast<const VKSwapchain *>(p)->GetHandle(); });

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkSwapchainKHR> swapchains;
    std::ranges::copy(std::span(presentInfo.pWaitSemaphores, presentInfo.waitSemaphoreCount) | v_semaphore,
                      std::back_inserter(waitSemaphores));
    std::ranges::copy(std::span(presentInfo.pSwapchains, presentInfo.swapchainCount) | v_swapchain,
                      std::back_inserter(swapchains));

    // std::vector<VkResult> results(swapchains.size());
    VkPresentInfoKHR info{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        static_cast<uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        static_cast<uint32_t>(swapchains.size()),
        swapchains.data(),
        presentInfo.pImageIndices,
    };
    // results.data()};
    auto res = vkQueuePresentKHR(mHandle, &info);

    switch (res) {
        case VK_SUCCESS:
            return Result::SUCCESS;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Result::SHIT_ERROR_OUT_OF_DATE;
        case VK_SUBOPTIMAL_KHR:
            return Result::SUBOPTIMAL;
        default:
            return Result::SHIT_ERROR;
    }
}
Result VKQueue::Present(const PresentInfo2 &presentInfo) {
    static auto v_semaphore =
        std::views::transform([](const Semaphore *p) { return static_cast<const VKSemaphore *>(p)->GetHandle(); });
    static auto v_swapchain =
        std::views::transform([](const Swapchain *p) { return static_cast<const VKSwapchain *>(p)->GetHandle(); });

    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkSwapchainKHR> swapchains;
    std::ranges::copy(presentInfo.waitSemaphores | v_semaphore, std::back_inserter(waitSemaphores));
    std::ranges::copy(presentInfo.swapchains | v_swapchain, std::back_inserter(swapchains));

    VkPresentInfoKHR info{
        VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        nullptr,
        static_cast<uint32_t>(waitSemaphores.size()),
        waitSemaphores.data(),
        static_cast<uint32_t>(swapchains.size()),
        swapchains.data(),
        presentInfo.imageIndices.data(),
    };
    auto res = vkQueuePresentKHR(mHandle, &info);

    switch (res) {
        case VK_SUCCESS:
            return Result::SUCCESS;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return Result::SHIT_ERROR_OUT_OF_DATE;
        case VK_SUBOPTIMAL_KHR:
            return Result::SUBOPTIMAL;
        default:
            return Result::SHIT_ERROR;
    }
}
void VKQueue::WaitIdle() { vkQueueWaitIdle(mHandle); }
}  // namespace Shit
