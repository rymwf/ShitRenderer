#include "common.hpp"
#include "model.hpp"

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	Image *envBRDFImage;
	ImageView *envBRDFImageView;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			.version = g_RendererVersion,
			.flags = RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT,
		};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window needed when using opengl
		WindowCreateInfo windowCreateInfo{
			{WindowCreateFlagBits::INVISIBLE},
			__FILE__,
			{{DEFAULT_WINDOW_X, DEFAULT_WINDOW_Y},
			 {DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT}}};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
		//1.5 choose phyiscal device
		//2. create device of a physical device
		DeviceCreateInfo deviceCreateInfo{};
		if ((g_RendererVersion & RendererVersion::TypeBitmask) == RendererVersion::GL)
			deviceCreateInfo = {window};

		device = renderSystem->CreateDevice(deviceCreateInfo);

		processImage();
	}

	void cleanUp()
	{
	}
	void run()
	{
		initRenderSystem();
	}

	void processImage()
	{
		//generateEnvBRDFMap(device, envBRDFImage, envBRDFImageView, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
		//envBRDFImage = generateImage2D(device, "opticalDepth.comp.spv", 64, 64, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
		envBRDFImage = generateEqirectangularProceduralSkybox(device, {-1, -1, 0}, 1024, 512, ImageLayout::SHADER_READ_ONLY_OPTIMAL);
		takeScreenshot(device, envBRDFImage, ImageLayout::GENERAL);
	}
};

int main(int ac, char **av)
{
	parseArgument(ac, av);
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