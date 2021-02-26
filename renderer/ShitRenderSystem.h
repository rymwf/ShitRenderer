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

		struct WindowContext
		{
			std::unique_ptr<ShitWindow> window;
			std::unique_ptr<Context> context;
		};
		std::vector<WindowContext> mWindowContexts;

		void DestroyWindow(const ShitWindow* pWindow);

		void ProcessWindowEvent(const Event& ev);

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

		virtual Context *CreateContext(const ContextCreateInfo &createInfo) = 0;

		/**
		 * @brief TODO: physical device not finished
		 * 
		 * @param physicalDevices 
		 */
		virtual void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) = 0;
	};

	RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
	void DeleteRenderSystem(RenderSystem *pRenderSystem);
} // namespace Shit
