#include "common.h"

uint32_t WIDTH = 800, HEIGHT = 600;

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Context *context;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			RendererVersion::GL,
			//RendererVersion::VULKAN,
			RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		WindowCreateInfo windowCreateInfo{
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}}};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);

		std::vector<PhysicalDevice> physicalDevices;
		renderSystem->EnumeratePhysicalDevice(physicalDevices);

		ContextCreateInfo contextCreateInfo{
			window,
		};
		context = renderSystem->CreateContext(contextCreateInfo);
	}
	void createScene()
	{
	}
	void mainLoop()
	{
		window->PollEvent();
	}
	void cleanUp()
	{
	}
	void run()
	{
		initRenderSystem();
		createScene();
		mainLoop();
		cleanUp();
	}
};

int main()
{
	Hello app;
	try
	{
		app.run();
	}
	catch (const std::exception &err)
	{
		std::cout << err.what() << std::endl;
	}
}