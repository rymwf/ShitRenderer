#include "common.hpp"

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
			// RendererVersion::VULKAN,
			RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window
		//auto func=std::make_shared<void()>
		WindowCreateInfo windowCreateInfo{
			{},
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}},
			std::make_shared<std::function<void(const Event&)>>(std::bind(&Hello::ProcessEvent, this, std::placeholders::_1)) };
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
	}

	/**
	 * @brief process window event, do not write render code here
	 * 
	 * @param ev 
	 */
	void ProcessEvent(const Event &ev)
	{
		std::visit(overloaded{
					   [](auto &&) {},
					   [&ev](const KeyEvent &value) {
						   if (value.keyCode == KeyCode::KEY_ESCAPE)
							   ev.pWindow->Close();
					   },
				   },
				   ev.value);
	}
	void mainLoop()
	{
		while (window->PollEvents())
		{
		}
	}
	void cleanUp()
	{
	}
	void run()
	{
		initRenderSystem();
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