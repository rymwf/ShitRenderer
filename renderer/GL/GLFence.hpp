/**
 * @file GLFence.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitFence.hpp>

#include "GLPrerequisites.hpp"
namespace Shit {
class GLFence : public Fence {
protected:
    mutable GLsync mHandle{};
    GLStateManager *mStateManager;

public:
    GLFence(GLStateManager *stateManager, const FenceCreateInfo &createInfo)
        : Fence(createInfo), mStateManager(stateManager) {
        if (!(createInfo.flags & FenceCreateFlagBits::SIGNALED_BIT))
            mHandle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    virtual ~GLFence() override { Delete(); }
    constexpr GLsync GetHandle() const { return mHandle; }
    constexpr void Delete() const {
        if (mHandle) glDeleteSync(mHandle);
        mHandle = 0;
    }
    constexpr bool IsSignaled() const {
        if (mHandle) {
            GLint value{};
            GLsizei length{1};
            glGetSynciv(mHandle, GL_SYNC_STATUS, 1, &length, &value);
            return value == GL_SIGNALED;
        }
        return true;
    }
    void Reset() const override { Delete(); }
    void Insert() const {
        Delete();
        mHandle = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
    Result WaitFor(uint64_t timeout) const override {
        if (!mHandle) return Result::SUCCESS;
        switch (glClientWaitSync(mHandle, GL_SYNC_FLUSH_COMMANDS_BIT, timeout)) {
            case GL_ALREADY_SIGNALED:
            case GL_CONDITION_SATISFIED:
                return Result::SUCCESS;
            case GL_TIMEOUT_EXPIRED:
                return Result::TIMEOUT;
            case GL_WAIT_FAILED:
            default:
                return Result::SHIT_ERROR;
        }
    }
    constexpr void Wait() const {
        if (mHandle) glWaitSync(mHandle, 0, GL_TIMEOUT_IGNORED);
    }
};
}  // namespace Shit