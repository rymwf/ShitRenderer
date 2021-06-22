/**
 * @file appbase.h
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "common.hpp"
#include "gui.hpp"
#include "scene.hpp"

#define CREATE_SCENE(x)             \
	int main(int argc, char **argv) \
	{                               \
		parseArgument(argc, argv);  \
		AppBase app(1280, 720);     \
		try                         \
		{                           \
			app.run<x>();           \
		}                           \
		catch (std::exception & e)  \
		{                           \
			LOG_VAR(e.what());      \
		}                           \
	}

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

constexpr SampleCountFlagBits SAMPLE_COUNT = SampleCountFlagBits::BIT_4;

/**
 * @brief cannot resize swapchain
 * 
 */
class AppBase
{
protected:
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	std::optional<QueueFamilyIndex> graphicsQueueFamilyIndex;
	QueueFamilyIndex presentQueueFamilyIndex;
	//std::optional<QueueFamilyIndex> transferQueueFamilyIndex;

	//=====================================
	//swapchains
	SwapchainCreateInfo swapchainCreateInfo;
	Swapchain *swapchain;
	std::vector<ImageView *> swapchainImageViews;
	std::vector<Image *> swapchainImages;
	std::vector<Framebuffer *> framebuffers;

	Queue *graphicsQueue;
	Queue *presentQueue;

	Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];

	RenderPass *defaultRenderPass;

	Image *depthImage;
	ImageView *depthImageView;

	Image *colorImage;
	ImageView *colorImageView;

	//========================

	CommandPool *defaultCommandPool;
	CommandPool *defaultResetableCommandPool;

	std::vector<CommandBuffer *> defaultPrimaryCommandBuffers;

	//============================================
	uint32_t FPS;
	float frameTimeInterval_ms;
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;
	//std::vector<Scene> scenes;
	//int sceneIndex{};
	//===========================

	bool startScreenshot{};

	//window rect
	uint32_t windowWidth;
	uint32_t windowHeight;

	//===================================================
	std::unique_ptr<Scene> scene;

	//=======
	std::unique_ptr<GUI> gui;

public:
	AppBase(uint32_t width, uint32_t height);

	template <typename T, typename = std::enable_if_t<std::is_base_of_v<Scene, T>>>
	void run()
	{
		scene = std::make_unique<T>();
		scene->Prepare();
		gui = std::make_unique<GUI>(scene.get());

		//add event listener
		window->AddEventListener(std::make_shared<std::function<void(const Event &)>>(std::bind(&AppBase::processEvent, this, std::placeholders::_1)));

		createDefaultCommandBuffers();
		//example
		mainloop();
	}

	RenderSystem *getRenderSystem() const { return renderSystem; }
	Device *getDevice() const { return device; }
	uint32_t getImageCount() const { return swapchain->GetImageNum(); }
	RenderPass* getDefaultRenderPass() const { return defaultRenderPass; }
	uint32_t getDefaultSubpass() const { return 0; }
	Framebuffer *getFramebuffer(uint32_t index) const { return framebuffers[index]; }
	constexpr ShitWindow *getWindow() const { return window; }

private:
	//==============
	void initRenderSystem();
	void createSwapchain();
	void createDepthResources();
	void createColorResources();
	void createRenderPasses();
	void createFramebuffers();

	void destroyWindowResouces();
	void recreateSwapchain();

	void createSyncObjects();

	void recordBackgroundSecondaryCommandBuffers(uint32_t imageIndex);

	void createDefaultCommandBuffers();

	void mainloop();

	void draw(uint32_t imageIndex);

	void processEvent(const Event &ev);
};
