#include "common.h"

uint32_t WIDTH = 800, HEIGHT = 600;

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			RendererVersion::GL,
			//RendererVersion::VULKAN,
			true};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		WindowCreateInfo windowCreateInfo{
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}}};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
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
		LOG_VAR(err.what());
	}
}