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
#include "GLContext.h"

#if _WIN32
#include <Windows.h>
#include <WGL/wgl.h>
#endif

namespace Shit
{
	class GLRenderSystem final : public RenderSystem
	{

	public:
		GLRenderSystem(const RenderSystemCreateInfo &createInfo) : RenderSystem(createInfo)
		{
			//			if (!gladLoadGL())
			//			{
			//				throw std::runtime_error("failed to init glad");
			//		}
		}
		~GLRenderSystem() override
		{
		}
		Context *CreateContext(const ContextCreateInfo &createInfo) override;
	};
}