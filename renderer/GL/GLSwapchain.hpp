/**
 * @file GLSwapchain.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include <renderer/ShitSwapchain.hpp>
#include "GLPrerequisites.hpp"

#ifdef _WIN32
#include <renderer/ShitWindowWin32.hpp>
#endif

namespace Shit
{

	/**
	 * @brief images count is in [8,16]
	 * 
	 */
	class GLSwapchain : public Swapchain
	{
		RenderPass *mpRenderPass{};
		Framebuffer *mpFramebuffer{};
		GLDevice *mpDevice;
		GLStateManager *mpStateManager;
		uint32_t mAvailableImageIndex{};
		bool mOutDated{};

		std::shared_ptr<std::function<void(const Event &)>> mProcessWindowEventCallable;

		std::vector<ImageView *> mpImageViews;

	protected:
		void CreateImages(uint32_t count);

		void CreateRenderPass();

		/**
		 * @brief Create a Framebuffer object used to blit
		 * 
		 */
		void CreateFramebuffer();

		constexpr uint32_t GetSwapchainImageCount() const
		{
			return (std::min)((std::max)(mCreateInfo.minImageCount, 2U), 8U);
		}

		void EnableDebugOutput(const void *userParam);

		void ProcessWindowEvent(const Event &ev);

	public:
		GLSwapchain(GLDevice *pDevice, GLStateManager *pStateManager, const SwapchainCreateInfo &createInfo);

		~GLSwapchain() override;

		Result GetNextImage(const GetNextImageInfo &info, uint32_t &index) override;

		void SwapBuffer() const;

		constexpr const Framebuffer *GetFramebufferPtr() const
		{
			return mpFramebuffer;
		}
		std::shared_ptr<std::function<void(const Event &)>> GetProcessWindowEventCallable() const
		{
			return mProcessWindowEventCallable;
		}
	};
} // namespace Shit
