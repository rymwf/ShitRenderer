/**
 * @file ShitDevice.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"

namespace Shit
{

	class Device
	{
	public:
		virtual ~Device() {}
		virtual std::optional<QueueFamilyIndex> GetPresentQueueFamilyIndex([[maybe_unused]] ShitWindow *window)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, INT_MAX}};
		}
		virtual std::optional<QueueFamilyIndex> GetQueueFamilyIndexByFlag(
			[[maybe_unused]] QueueFlagBits flags, [[maybe_unused]] const std::unordered_set<uint32_t> &skipIndices)
		{
			//for opengl
			return std::optional<QueueFamilyIndex>{{0, INT_MAX}};
		}
	};
}
