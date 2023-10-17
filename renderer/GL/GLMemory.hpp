/**
 * @file GLMemory.hpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#pragma once
#include <renderer/ShitMemory.hpp>

#include "GLPrerequisites.hpp"

namespace Shit {
class GLBuffer;
class GLImage;
class GLMemory final : public Memory {
    std::variant<GLBuffer *, GLImage *> mTarget;

public:
    GLMemory(const std::variant<GLBuffer *, GLImage *> &target) : mTarget(target), Memory({}) {}
};
}  // namespace Shit
