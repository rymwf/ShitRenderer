/**
 * @file GLRenderSystem.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.h>

#include "GLPrerequisites.h"
#include "GLDevice.h"

namespace Shit
{

	class GLRenderSystem final : public RenderSystem
	{

	public:
		GLRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo)
		{
		}
		~GLRenderSystem() override
		{
		}

		void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) override;

		Device *CreateDevice(const DeviceCreateInfo &createInfo) override
		{
#ifdef _WIN32
			mDevices.emplace_back(std::make_unique<GLDeviceWin32>(createInfo.pWindow, mCreateInfo));
#else
			static_assert(0, "GL CreateDevice is not implemented yet");
#endif
			return mDevices.back().get();
		};
	};
}