/**
 * @file GLRenderSystem.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitRenderSystem.hpp>
#include "GLPrerequisites.hpp"

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

		Device *CreateDevice(const DeviceCreateInfo &createInfo) override;
	};
}