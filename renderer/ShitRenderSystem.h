/**
 * @file ShitRenderer.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.h"
#include "ShitTypes.h"
#include "ShitNonCopyable.h"
#include "ShitWindow.h"
#include "ShitContext.h"

namespace Shit
{
	class RenderSystem : public NonCopyable
	{
	protected:
		RenderSystemCreateInfo mCreateInfo;

		RenderSystem() {}

		struct WindowSurface
		{
			std::unique_ptr<ShitWindow> window;
			std::unique_ptr<Context> surface;
		};
		std::vector<WindowSurface> mWindowSurfaces;

		virtual Context *CreateContext(const ContextCreateInfo &createInfo) = 0;

		
	public:
		RenderSystem(const RenderSystemCreateInfo &createInfo)
			: mCreateInfo(createInfo)
		{
		}
		virtual ~RenderSystem()
		{
		}

		const RenderSystemCreateInfo *GetCreateInfo() const
		{
			return &mCreateInfo;
		}

		ShitWindow *CreateRenderWindow(const WindowCreateInfo &createInfo);

//		virtual void EnumeratePhysicalDevice() ;
	};

	RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
	void DeleteRenderSystem(RenderSystem *pRenderSystem);
} // namespace Shit
