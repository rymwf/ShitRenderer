/**
 * @file VKContext.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "VKPrerequisites.h"
#include <renderer/ShitContext.h>

namespace Shit
{
	class VKContext final : public Context
	{
		VkSurfaceKHR mHandle;

	public:
		VKContext(VkInstance instance, const ContextCreateInfo &createInfo);

	};

} // namespace Shit
