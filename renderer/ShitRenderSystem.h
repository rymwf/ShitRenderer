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
#include "ShitNonCopyable.h"
#include "ShitWindow.h"
#include "ShitDevice.h"

namespace Shit
{
	class RenderSystem : public NonCopyable
	{
	protected:
		RenderSystemCreateInfo mCreateInfo;

		std::vector<std::unique_ptr<ShitWindow>> mWindows;
		std::vector<std::unique_ptr<Device>> mDevices;

	protected:
		RenderSystem() {}

		void DestroyDevice(const Device *pDevice);

		virtual std::shared_ptr<Surface> CreateSurface([[maybe_unused]] const SurfaceCreateInfo &createInfo, [[maybe_unused]] ShitWindow *pWindow)
		{
			return std::move(nullptr);
		};

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

		/**
		 * @brief Create a Device object, 
		 * create a device include all queue families supported by the physical device
		 * currently physical device selection is not supported, the method will use the current gpu(opengl) or the best gpu(Vulkan)
		 * 
		 * @param createInfo 
		 * @return Device* 
		 */
		virtual Device *CreateDevice(const DeviceCreateInfo &createInfo) = 0;

		/**
		 * @brief TODO: physical device not finished
		 * 
		 * @param physicalDevices 
		 */
		virtual void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) = 0;

		friend class ShitWindow;
	};

	SHIT_API RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
} // namespace Shit
