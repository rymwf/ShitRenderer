#include "common.h"
#include <GL/glew.h>

uint32_t WIDTH = 800, HEIGHT = 600;

const char *vertShaderName = "01.vert.spv";
const char *fragShaderName = "01.frag.spv";

std::string vertShaderPath;
std::string fragShaderPath;

constexpr Shit::RendererVersion rendererVersion{Shit::RendererVersion::VULKAN};
//constexpr Shit::RendererVersion rendererVersion{Shit::RendererVersion::GL};

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	std::optional<QueueFamilyIndex> graphicsQueueFamilyIndex;
	QueueFamilyIndex presentQueueFamilyIndex;
	std::optional<QueueFamilyIndex> transferQueueFamilyIndex;

	Swapchain *swapchain;
	std::vector<ImageView *> swapchainImageViews;
	std::vector<Image *> swapchainImages;
	std::vector<Framebuffer*> framebuffers;

	CommandPool *commandPool;
	std::vector<CommandBuffer *> commandBuffers;
	Queue *graphicsQueue;

	Shader *vertShader;
	Shader *fragShader;

	DescriptorSetLayout *descriptorSetLayout;
	PipelineLayout *pipelineLayout;
	RenderPass *renderPass;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			rendererVersion,
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

		//3
		graphicsQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::GRAPHICS_BIT, {});
		if (!graphicsQueueFamilyIndex.has_value())
			THROW("failed to find a graphic queue");

		LOG_VAR(graphicsQueueFamilyIndex->index);
		LOG_VAR(graphicsQueueFamilyIndex->count);

		transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index, presentQueueFamilyIndex.index});
		if (!transferQueueFamilyIndex.has_value())
			THROW("failed to find a transfer queue");

		LOG_VAR(transferQueueFamilyIndex->index);
		LOG_VAR(transferQueueFamilyIndex->count);

		//4. command pool
		CommandPoolCreateInfo commandPoolCreateInfo{
			{},
			graphicsQueueFamilyIndex->index};
		commandPool = device->CreateCommandPool(commandPoolCreateInfo);

		//5
		QueueCreateInfo queueCreateInfo{
			graphicsQueueFamilyIndex->index,
			0,
		};
		graphicsQueue = device->CreateDeviceQueue(queueCreateInfo);
		createShaders();
		createDescriptors();

		createSwapchains();
		createPipeline();
		createRenderPasses();
		createFramebuffers();

		createCommandBuffers();
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
	}
	void run()
	{
		initRenderSystem();
		mainLoop();
		cleanUp();
	}

	void drawFrame()
	{
	}

	void createShaders()
	{
		vertShaderPath = buildShaderPath(vertShaderName, rendererVersion);
		fragShaderPath = buildShaderPath(fragShaderName, rendererVersion);

		std::string vertCode = readFile(vertShaderPath.c_str());
		std::string fragCode = readFile(fragShaderPath.c_str());

		ShaderCreateInfo vertShaderCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			vertCode};

		ShaderCreateInfo fragShaderCreateInfo{
			ShaderStageFlagBits::FRAGMENT_BIT,
			fragCode};

		vertShader = device->CreateShader(vertShaderCreateInfo);
		fragShader = device->CreateShader(fragShaderCreateInfo);
	}

	void createVertexBuffer()
	{
	}
	void createSwapchains()
	{
		SwapchainCreateInfo swapchainCreateInfo{
			2,
			ShitFormat::BGRA8_SRGB,
			ColorSpace::SRGB_NONLINEAR,
			{800, 600},
			1,
			PresentMode::FIFO};
		swapchain = device->CreateSwapchain(swapchainCreateInfo, window);
		swapchain->GetImages(swapchainImages);
		ImageViewCreateInfo imageViewCreateInfo{
			nullptr,
			ImageViewType::TYPE_2D,
			swapchainCreateInfo.format,
			{},
			{0, 1, 0, 1}};
		for (auto &&e : swapchainImages)
		{
			imageViewCreateInfo.pImage = e;
			swapchainImageViews.emplace_back(device->CreateImageView(imageViewCreateInfo));
		}

		presentQueueFamilyIndex = swapchain->GetPresentQueueFamilyIndex();

		LOG_VAR(presentQueueFamilyIndex.index);
		LOG_VAR(presentQueueFamilyIndex.count);
	}
	void createDescriptors()
	{
		descriptorSetLayout = device->CreateDescriptorSetLayout({});
	}
	void createRenderPasses()
	{
		std::vector<AttachmentDescription> attachmentDescriptions{
			{swapchain->GetCreateInfoPtr()->format,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,
			 AttachmentStoreOp::DONT_CARE,
			 AttachmentLoadOp::DONT_CARE,
			 AttachmentStoreOp::DONT_CARE,
			 ImageLayout::COLOR_ATTACHMENT}};

		std::vector<AttachmentReference> colorAttachments{
			{0, //the index of attachment description
			 ImageLayout::COLOR_ATTACHMENT},
		};

		std::vector<SubpassDescription> subPasses{
			{PipelineBindPoint::GRAPHICS,
			 colorAttachments}};

		RenderPassCreateInfo renderPassCreateInfo{
			attachmentDescriptions,
			subPasses};

		renderPass = device->CreateRenderPass(renderPassCreateInfo);
	}
	void createPipeline()
	{
		pipelineLayout = device->CreatePipelineLayout({});

		PipelineShaderStageCreateInfo shaderStageCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			vertShader,
			"main",
		};
		GraphicsPipelineCreateInfo pipelineCreateInfo{
			std::make_shared<std::vector<PipelineShaderStageCreateInfo>>(
				std::vector<PipelineShaderStageCreateInfo>{shaderStageCreateInfo}),
		};

		QueueCreateInfo queueCreateInfo{};

		graphicsQueue = device->CreateDeviceQueue(queueCreateInfo);

		device->CreateGraphicsPipeline(pipelineCreateInfo);
	}
	void createFramebuffers()
	{
		FramebufferCreateInfo framebufferCreateInfo{
			renderPass,
			{},
			{800, 600},
			1};
		for(auto&& e:swapchainImageViews)
		{
			framebufferCreateInfo.attachments = {e};
			framebuffers.emplace_back(device->CreateFramebuffer(framebufferCreateInfo));
		}
	}
	void createCommandBuffers()
	{
		CommandBufferCreateInfo cmdBufferCreateInfo{
			CommandBufferLevel::PRIMARY};
		commandPool->CreateCommandBuffers(cmdBufferCreateInfo, commandBuffers);
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