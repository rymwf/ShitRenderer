/**
 * @file GLDevice.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once

#include <renderer/ShitDevice.hpp>
#include "GLPrerequisites.hpp"
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Shit
{
	/**
	 * @brief in opengl, device is created based on windows, one window has its own device
	 * 
	 */
	class GLDevice : public Device
	{
	protected:
		GLStateManager mStateManager{};

		RenderSystemCreateInfo mRenderSystemCreateInfo;

		ShitWindow* mpWindow{};

		void EnableDebugOutput(const void *userParam);

		virtual void CreateRenderContext() = 0;

	public:
		GLDevice(const DeviceCreateInfo &createInfo, const RenderSystemCreateInfo &renderSystemCreateInfo);

		Result WaitIdle() override;

		constexpr GLStateManager *GetStateManager()
		{
			return &mStateManager;
		}
		virtual void MakeCurrent() const = 0;

		virtual void SetPresentMode(PresentMode mode) const = 0;

		virtual void SwapBuffer() const = 0;

		Swapchain *Create(const SwapchainCreateInfo &createInfo, ShitWindow *pWindow) override;

		CommandPool *Create(const CommandPoolCreateInfo &createInfo) override;

		Shader *Create(const ShaderCreateInfo &createInfo) override;

		Pipeline *Create(const GraphicsPipelineCreateInfo &createInfo) override;

		Pipeline *Create(const ComputePipelineCreateInfo &createInfo) override;

		Queue *Create(const QueueCreateInfo &createInfo) override;

		Buffer *Create(const BufferCreateInfo &createInfo, const void *pData) override;

		Image *Create(const ImageCreateInfo &createInfo, const void *pData) override;

		DescriptorSetLayout *Create(const DescriptorSetLayoutCreateInfo &createInfo) override;

		ImageView *Create(const ImageViewCreateInfo &createInfo) override;
		PipelineLayout *Create(const PipelineLayoutCreateInfo &createInfo) override;
		RenderPass *Create(const RenderPassCreateInfo &createInfo) override;
		Framebuffer *Create(const FramebufferCreateInfo &createInfo) override;
		Semaphore *Create(const SemaphoreCreateInfo &createInfo) override;
		Fence *Create(const FenceCreateInfo &createInfo) override;
		Sampler *Create(const SamplerCreateInfo &createInfo) override;
		DescriptorPool *Create(const DescriptorPoolCreateInfo &createInfo) override;

		void UpdateDescriptorSets(const std::vector<WriteDescriptorSet> &descriptorWrites, const std::vector<CopyDescriptorSet> &descriptorCopies) override;
	};

#ifdef _WIN32
	class GLDeviceWin32 final : public GLDevice
	{
		HDC mHDC;
		HGLRC mHRenderContext; //false context

		void CreateRenderContext() override;

		void SetPresentMode(PresentMode mode) const override;

		/**
		 * @brief create a false rendercontext and init wgl extensions
		 * 
		 */
		void InitWglExtentions();
	public:
		GLDeviceWin32(const DeviceCreateInfo &createInfo, const RenderSystemCreateInfo &renderSystemCreateInfo);
		~GLDeviceWin32() override
		{
			wglDeleteContext(mHRenderContext);
		}

		void MakeCurrent() const override;

		void SwapBuffer() const override;

		constexpr HDC GetHDC() const
		{
			return mHDC;
		}
		constexpr HGLRC GetRenderContext() const
		{
			return mHRenderContext;
		}

		void GetWindowPixelFormats(const ShitWindow *pWindow, std::vector<WindowPixelFormat> &formats) override;

		void GetPresentModes(const ShitWindow *pWindow, std::vector<PresentMode> &presentModes) override;
	};
#endif

}