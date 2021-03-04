#include "common.h"
#include <GL/glew.h>

uint32_t WIDTH = 800, HEIGHT = 600;

const char *vertShaderName = "01.vert.spv";
const char *fragShaderName = "01.frag.spv";

std::string vertShaderPath;
std::string fragShaderPath;

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;
	Swapchain *swapchain;
	Shader *vertShader;
	Shader *fragShader;
	Queue* graphicsQueue;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			//RendererVersion::GL,
			RendererVersion::VULKAN,
			RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window
		WindowCreateInfo windowCreateInfo{
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}},
			std::bind(&Hello::ProcessEvent, this, std::placeholders::_1)};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
		//1.5 choose phyiscal device
		//2. create device of a physical device
		DeviceCreateInfo deviceCreateInfo;
		deviceCreateInfo.pWindow = window;
		device = renderSystem->CreateDevice(deviceCreateInfo);

		//3. create swapchain
		SwapchainCreateInfo swapchainCreateInfo{
			device,
			window,
			2,
			ShitFormat::BGRA8_SRGB,
			ColorSpace::SRGB_NONLINEAR,
			{800, 600},
			1,
			PresentMode::FIFO};
		swapchain = renderSystem->CreateSwapchain(swapchainCreateInfo);

		QueueCreateInfo queueCreateInfo{
			device,
			QueueFlagBits::GRAPHICS_BIT,
			0,
		};
		graphicsQueue = renderSystem->CreateDeviceQueue(queueCreateInfo);

		vertShaderPath = buildShaderPath(vertShaderName, renderSystemCreateInfo.version);
		fragShaderPath = buildShaderPath(fragShaderName, renderSystemCreateInfo.version);

		createPipeline();
	}
	/**
	 * @brief process window event, do not write render code here
	 * 
	 * @param ev 
	 */
	void ProcessEvent(const Event &ev)
	{
		switch (ev.type)
		{
		case EventType::KEYBOARD:
			if (ev.key.keyCode == KeyCode::KEY_ESCAPE)
				ev.pWindow->Close();
			break;
		}
	}

	void mainLoop()
	{
		while (window->PollEvent())
		{
			drawFrame();
		}
	}
	void cleanUp()
	{
		renderSystem->DestroyShader(vertShader);
		renderSystem->DestroyShader(fragShader);
	}
	void run()
	{
		initRenderSystem();
		mainLoop();
		cleanUp();
	}

	void drawFrame()
	{
		//glClear(GL_COLOR_BUFFER_BIT);
	}

	void createPipeline()
	{

		std::string vertCode = readFile(vertShaderPath.c_str());
		std::string fragCode = readFile(fragShaderPath.c_str());

		ShaderCreateInfo vertShaderCreateInfo{
			device,
			ShaderStageFlagBits::VERTEX_BIT,
			vertCode};

		ShaderCreateInfo fragShaderCreateInfo{
			device,
			ShaderStageFlagBits::FRAGMENT_BIT,
			fragCode};

		vertShader = renderSystem->CreateShader(vertShaderCreateInfo);
		fragShader = renderSystem->CreateShader(fragShaderCreateInfo);

		PipelineShaderStageCreateInfo shaderStageCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			vertShader,
			"main",
		};
		GraphicsPipelineCreateInfo pipelineCreateInfo{
			nullptr,
			std::make_shared<std::vector<PipelineShaderStageCreateInfo>>(
				std::vector<PipelineShaderStageCreateInfo>{shaderStageCreateInfo}),
		};

		QueueCreateInfo queueCreateInfo
		{
			device,

		};

		graphicsQueue=renderSystem->CreateDeviceQueue(queueCreateInfo);

		renderSystem->CreateGraphicsPipeline(pipelineCreateInfo);
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