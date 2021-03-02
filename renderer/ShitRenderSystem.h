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
#include "ShitSwapchain.h"

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

		virtual void ProcessWindowEvent(const Event &ev) = 0;

		virtual void CreateSurface([[maybe_unused]] ShitWindow *pWindow){};

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
		 * @param pPhyicalDevice not used, can be nullptr
		 * @param pWindow (opengl, used to create a false context) ,for vulkan canbe nullptr
		 * @return Device* 
		 */
		virtual Device *CreateDevice(PhysicalDevice *pPhyicalDevice, ShitWindow *pWindow) = 0;

		/**
		 * @brief TODO: physical device not finished
		 * 
		 * @param physicalDevices 
		 */
		virtual void EnumeratePhysicalDevice(std::vector<PhysicalDevice> &physicalDevices) = 0;


		/**
		 * @brief Create a Swapchain object,
		 *  for opengl, just create a render context
		 * 
		 * @param createInfo 
		 * @return Swapchain* 
		 */
		virtual Swapchain *CreateSwapchain(const SwapchainCreateInfo &createInfo) = 0;
	};

	RenderSystem *LoadRenderSystem(const RenderSystemCreateInfo &createInfo);
	void DeleteRenderSystem(RenderSystem *pRenderSystem);
} // namespace Shit
