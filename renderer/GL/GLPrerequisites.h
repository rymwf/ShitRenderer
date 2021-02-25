/**
 * @file GLPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#ifdef _WIN32
#define APIENTRY __stdcall
#endif
#include <GL/glew.h>

#if _WIN32
#include <GL/wglew.h>
#endif

#include <renderer/ShitRendererPrerequisites.h>

namespace Shit
{

} // namespace Shit
