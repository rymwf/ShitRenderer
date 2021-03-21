/**
 * @file GLSwapchain.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSwapchain.h>
#include "GLPrerequisites.h"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.h>
#endif

namespace Shit
{

	/**
	 * @brief images count is in [8,16]
	 * 
	 */
	class GLSwapchain : public Swapchain
	{
		Framebuffer *mpFramebuffer{};
		GLDevice *mpDevice;
		GLStateManager *mpStateManager;
		uint32_t mAvailableImageIndex{};
		bool mOutDated{};

		std::shared_ptr<std::function<void(const Event &)>> mProcessWindowEventCallable;

	protected:
		void CreateImages(uint32_t count);

		/**
		 * @brief Create a Framebuffer object used to blit
		 * 
		 */
		void CreateFramebuffer();

		constexpr uint32_t GetSwapchainImageCount() const
		{
			return (std::min)((std::max)(mCreateInfo.minImageCount, 8U), 8U);
		}

		void EnableDebugOutput(const void *userParam);

		void ProcessWindowEvent(const Event &ev);

	public:
		GLSwapchain(GLDevice *pDevice, GLStateManager *pStateManager, const SwapchainCreateInfo &createInfo);

		~GLSwapchain() override;

		Result GetNextImage(const GetNextImageInfo &info, uint32_t& index) override;

		void SwapBuffer() const;

		constexpr const Framebuffer *GetFramebufferPtr() const
		{
			return mpFramebuffer;
		}
		std::shared_ptr<std::function<void(const Event &)>> GetProcessWindowEventCallable()
		{
			return mProcessWindowEventCallable;
		}
	};
} // namespace Shit
