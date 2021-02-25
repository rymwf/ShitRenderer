/**
 * @file VKPrerequisites.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRendererPrerequisites.h>

#include <vulkan/vulkan.h>
#if _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif

constexpr char *LAYER_VALIDATION_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";