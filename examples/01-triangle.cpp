#include "common.hpp"

uint32_t WIDTH = 800, HEIGHT = 600;

const char *vertShaderName = "01.vert.spv";
const char *fragShaderName = "01.frag.spv";

std::string vertShaderPath;
std::string fragShaderPath;

std::vector<uint16_t> indices{0, 1, 3, 0, 2, 3};

class Hello
{
	RenderSystem *renderSystem;
	ShitWindow *window;
	Device *device;

	std::optional<QueueFamilyIndex> graphicsQueueFamilyIndex;
	QueueFamilyIndex presentQueueFamilyIndex;
	std::optional<QueueFamilyIndex> transferQueueFamilyIndex;

	SwapchainCreateInfo swapchainCreateInfo;
	Swapchain *swapchain;
	std::vector<ImageView *> swapchainImageViews;
	std::vector<Image *> swapchainImages;
	std::vector<Framebuffer *> framebuffers;

	CommandPool *commandPool;
	std::vector<CommandBuffer *> commandBuffers;
	Queue *graphicsQueue;
	Queue *presentQueue;
	Semaphore *imageAvailableSemaphore;
	Semaphore *renderFinishedSemaphore;

	Shader *vertShader;
	Shader *fragShader;

	DescriptorSetLayout *descriptorSetLayout;
	PipelineLayout *pipelineLayout;
	RenderPass *renderPass;
	Pipeline *pipeline;

	Buffer *drawIndirectCmdBuffer;
	Buffer *drawCountBuffer;
	Buffer *drawIndexedIndirectCmdBuffer;
	Buffer *indexBuffer;

public:
	void initRenderSystem()
	{
		RenderSystemCreateInfo renderSystemCreateInfo{
			rendererVersion,
			RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT};

		renderSystem = LoadRenderSystem(renderSystemCreateInfo);
		//1. create window
		WindowCreateInfo windowCreateInfo{
			{},
			__FILE__,
			{{DEFAULT_WINDOW_X, DEFAULT_WINDOW_Y},
			 {DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT}},
			std::make_shared<std::function<void(const Event &)>>(std::bind(&Hello::ProcessEvent, this, std::placeholders::_1))};
		window = renderSystem->CreateRenderWindow(windowCreateInfo);
		//1.5 choose phyiscal device
		//2. create device of a physical device
		DeviceCreateInfo deviceCreateInfo{};
		if ((rendererVersion & RendererVersion::TypeBitmask) == RendererVersion::GL)
			deviceCreateInfo = {window};

		device = renderSystem->CreateDevice(deviceCreateInfo);

		//3
		graphicsQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::GRAPHICS_BIT, {});
		if (!graphicsQueueFamilyIndex.has_value())
			THROW("failed to find a graphic queue");

		LOG_VAR(graphicsQueueFamilyIndex->index);
		LOG_VAR(graphicsQueueFamilyIndex->count);

		//4. command pool
		CommandPoolCreateInfo commandPoolCreateInfo{
			{},
			graphicsQueueFamilyIndex->index};
		commandPool = device->Create(commandPoolCreateInfo);

		//5
		QueueCreateInfo queueCreateInfo{
			graphicsQueueFamilyIndex->index,
			0,
		};
		graphicsQueue = device->Create(queueCreateInfo);

		createShaders();
		createDescriptors();
		createDrawCommandBuffers();
		createSyncObjects();
		createIndexBuffer();

		recreateSwapchain();

		transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index, presentQueueFamilyIndex.index});
		if (!transferQueueFamilyIndex.has_value())
			THROW("failed to find a transfer queue");

		LOG_VAR(transferQueueFamilyIndex->index);
		LOG_VAR(transferQueueFamilyIndex->count);
	}
	/**
	 * @brief process window event, do not write render code here
	 * 
	 * @param ev 
	 */
	void ProcessEvent(const Event &ev)
	{
		std::visit(overloaded{
					   [&ev](const KeyEvent &value) {
						   if (value.keyCode == KeyCode::KEY_ESCAPE)
							   ev.pWindow->Close();
					   },
					   [](auto &&) {},
					   [&ev]([[maybe_unused]] const WindowResizeEvent &value) {
					   },
				   },
				   ev.value);
	}

	void mainLoop()
	{
		while (window->PollEvents())
		{
			drawFrame();
		}
		presentQueue->WaitIdle();
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
	void cleanupSwapchain()
	{
		for (auto &&e : swapchainImageViews)
			device->Destroy(e);
		device->Destroy(renderPass);
		for (auto &&e : framebuffers)
			device->Destroy(e);
		device->Destroy(pipelineLayout);
		device->Destroy(pipeline);
		for (auto &&e : commandBuffers)
			commandPool->DestroyCommandBuffer(e);
		device->Destroy(swapchain);
	}
	void recreateSwapchain()
	{
		cleanupSwapchain();
		createSwapchains();
		createRenderPasses();
		createFramebuffers();
		createPipeline();
		createCommandBuffers();
	}
	void drawFrame()
	{
		//static uint32_t currentFrame{};
		uint32_t imageIndex{};
		GetNextImageInfo nextImageInfo{
			UINT64_MAX,
			imageAvailableSemaphore};
		auto ret = swapchain->GetNextImage(nextImageInfo, imageIndex);
		if (ret == Result::SHIT_ERROR_OUT_OF_DATE)
		{
			presentQueue->WaitIdle();
			recreateSwapchain();
			return;
		}
		else if (ret != Result::SUCCESS)
		{
			THROW("failed to get next image");
		}

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphore},
			 {commandBuffers[imageIndex]},
			 {renderFinishedSemaphore}}};
		graphicsQueue->Submit(submitInfos, nullptr);

		PresentInfo presentInfo{
			{renderFinishedSemaphore},
			{swapchain},
			{imageIndex}};
		auto res = presentQueue->Present(presentInfo);
		if (res == Result::SHIT_ERROR_OUT_OF_DATE)
		{
			presentQueue->WaitIdle();
			recreateSwapchain();
		}
		else if (res != Result::SUCCESS)
		{
			THROW("failed to present swapchain image");
		}
		presentQueue->WaitIdle();
	}

	void createShaders()
	{
		vertShaderPath = buildShaderPath(vertShaderName, rendererVersion);
		fragShaderPath = buildShaderPath(fragShaderName, rendererVersion);

		std::string vertCode = readFile(vertShaderPath.c_str());
		std::string fragCode = readFile(fragShaderPath.c_str());

		ShaderCreateInfo vertShaderCreateInfo{vertCode};
		ShaderCreateInfo fragShaderCreateInfo{fragCode};

		vertShader = device->Create(vertShaderCreateInfo);
		fragShader = device->Create(fragShaderCreateInfo);
	}

	void createSwapchains()
	{
		auto swapchainFormat = chooseSwapchainFormat(
			{
				{ShitFormat::BGRA8_SRGB, ColorSpace::SRGB_NONLINEAR},
				{ShitFormat::RGBA8_SRGB, ColorSpace::SRGB_NONLINEAR},
			},
			device,
			window);
		LOG_VAR(static_cast<int>(swapchainFormat.format));
		LOG_VAR(static_cast<int>(swapchainFormat.colorSpace));

		auto presentMode = choosePresentMode({PresentMode::IMMEDIATE, PresentMode::FIFO}, device, window);

		swapchainCreateInfo = SwapchainCreateInfo{
			2,
			swapchainFormat.format,
			swapchainFormat.colorSpace,
			{800, 600},
			1,
			ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
			presentMode};
		window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
		while (swapchainCreateInfo.imageExtent.width == 0 && swapchainCreateInfo.imageExtent.height == 0)
		{
			window->WaitEvents();
			window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
		}
		swapchain = device->Create(swapchainCreateInfo, window);
		swapchain->GetImages(swapchainImages);
		ImageViewCreateInfo imageViewCreateInfo{
			nullptr,
			ImageViewType::TYPE_2D,
			swapchainCreateInfo.format,
			{},
			{0, 1, 0, 1}};
		auto swapchainImageCount = swapchainImages.size();
		swapchainImageViews.resize(swapchainImageCount);
		while (swapchainImageCount-- > 0)
		{
			imageViewCreateInfo.pImage = swapchainImages[swapchainImageCount];
			swapchainImageViews[swapchainImageCount] = device->Create(imageViewCreateInfo);
		}
		presentQueueFamilyIndex = swapchain->GetPresentQueueFamilyIndex();

		LOG_VAR(presentQueueFamilyIndex.index);
		LOG_VAR(presentQueueFamilyIndex.count);

		QueueCreateInfo queueCreateInfo{
			presentQueueFamilyIndex.index,
			0,
		};
		presentQueue = device->Create(queueCreateInfo);
	}
	void createDescriptors()
	{
		descriptorSetLayout = device->Create(DescriptorSetLayoutCreateInfo{});
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
			 ImageLayout::UNDEFINED,	   //inital layout
			 ImageLayout::PRESENT_SRC}};

		std::vector<AttachmentReference> colorAttachments{
			{0, //the index of attachment description
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};

		std::vector<SubpassDescription> subPasses{
			SubpassDescription{
				.pipelineBindPoint = PipelineBindPoint::GRAPHICS,
				.colorAttachments = colorAttachments,
			},
		};

		RenderPassCreateInfo renderPassCreateInfo{
			attachmentDescriptions,
			subPasses};

		renderPass = device->Create(renderPassCreateInfo);
	}
	void createPipeline()
	{
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{});
		std::vector<PipelineShaderStageCreateInfo> shaderStageCreateInfos{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::VERTEX_BIT,
				vertShader,
				"main",
			},
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::FRAGMENT_BIT,
				fragShader,
				"main",
			},
		};
		VertexInputStateCreateInfo vertexInputState{};
		PipelineInputAssemblyStateCreateInfo inputAssemblyState{
			PrimitiveTopology::TRIANGLE_LIST,
		};
		PipelineViewportStateCreateInfo viewportStateCreateInfo{
			{{0, 0, static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width), static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height), 0, 1}},
			{{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent}}};
		PipelineTessellationStateCreateInfo tessellationState{};
		PipelineRasterizationStateCreateInfo rasterizationState{
			false,
			false,
			PolygonMode::FILL,
			CullMode::BACK,
			FrontFace::COUNTER_CLOCKWISE,
			false,
		};
		rasterizationState.lineWidth = 1.f;

		PipelineMultisampleStateCreateInfo multisampleState{
			SampleCountFlagBits::BIT_1,
		};
		PipelineDepthStencilStateCreateInfo depthStencilState{};

		PipelineColorBlendAttachmentState colorBlendAttachmentstate{};
		colorBlendAttachmentstate.colorWriteMask = ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT;
		PipelineColorBlendStateCreateInfo colorBlendState{
			false,
			{},
			{std::move(colorBlendAttachmentstate)},
		};

		GraphicsPipelineCreateInfo pipelineCreateInfo{
			std::move(shaderStageCreateInfos),
			std::move(vertexInputState),
			std::move(inputAssemblyState),
			std::move(viewportStateCreateInfo),
			std::move(tessellationState),
			std::move(rasterizationState),
			std::move(multisampleState),
			std::move(depthStencilState),
			std::move(colorBlendState),
			{},
			pipelineLayout,
			renderPass,
			0};

		pipeline = device->Create(pipelineCreateInfo);
	}
	void createFramebuffers()
	{
		FramebufferCreateInfo framebufferCreateInfo{
			renderPass,
			{},
			swapchain->GetCreateInfoPtr()->imageExtent,
			1};
		auto count = swapchainImageViews.size();
		framebuffers.resize(count);
		while (count-- > 0)
		{
			framebufferCreateInfo.attachments = {swapchainImageViews[count]};
			framebuffers[count] = device->Create(framebufferCreateInfo);
		}
	}
	void createCommandBuffers()
	{
		uint32_t count = static_cast<uint32_t>(swapchainImageViews.size());
		CommandBufferCreateInfo cmdBufferCreateInfo{
			CommandBufferLevel::PRIMARY, count};
		commandPool->CreateCommandBuffers(cmdBufferCreateInfo, commandBuffers);

		CommandBufferBeginInfo cmdBufferBeginInfo{};

		std::vector<ClearValue> clearValues{std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f}};

		RenderPassBeginInfo renderPassBeginInfo{
			renderPass,
			nullptr,
			Rect2D{
				{},
				swapchain->GetCreateInfoPtr()->imageExtent},
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data(),
			SubpassContents::INLINE};

		for (uint32_t i = 0; i < count; ++i)
		{
			renderPassBeginInfo.pFramebuffer = framebuffers[i];
			commandBuffers[i]->Begin(cmdBufferBeginInfo);
			commandBuffers[i]->BeginRenderPass(renderPassBeginInfo);
			commandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, pipeline});

			int drawMethod = 3;
			switch (drawMethod)
			{
			case 0:
			default:
			{
				DrawIndirectCommand drawIndirectCmd{4, 1, 0, 0};
				commandBuffers[i]->Draw(drawIndirectCmd);
			}
			break;
			case 1:
			{
				DrawIndirectInfo drawCmdInfo{
					drawIndirectCmdBuffer,
					0,
					1,
					sizeof(DrawIndirectCommand)};
				commandBuffers[i]->DrawIndirect(drawCmdInfo);
			}
			break;
			case 2:
			{
				DrawIndirectCountInfo drawCmdInfo{
					drawIndirectCmdBuffer,
					0,
					drawCountBuffer,
					0,
					1,
					sizeof(DrawIndirectCommand)};
				commandBuffers[i]->DrawIndirectCount(drawCmdInfo);
			}
			break;
			case 3:
			{
				//draw index
				commandBuffers[i]->BindIndexBuffer(BindIndexBufferInfo{
					indexBuffer,
					0,
					IndexType::UINT16});
				DrawIndexedIndirectCommand drawIndexedIndirectCmd{static_cast<uint32_t>(indices.size()), 1, 0, 0, 0};
				commandBuffers[i]->DrawIndexed(drawIndexedIndirectCmd);
			}
			break;
			}
			commandBuffers[i]->EndRenderPass();
			commandBuffers[i]->End();
		}
	}

	void createSyncObjects()
	{
		imageAvailableSemaphore = device->Create(SemaphoreCreateInfo{});
		renderFinishedSemaphore = device->Create(SemaphoreCreateInfo{});
	}
	void createDrawCommandBuffers()
	{
		//create draw indirect command buffer
		std::vector<DrawIndirectCommand> drawIndirectCmds{{4, 1, 0, 0}};
		BufferCreateInfo bufferCreateInfo{
			{},
			sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndirectCmdBuffer = device->Create(bufferCreateInfo, drawIndirectCmds.data());
		bufferCreateInfo.size = sizeof(uint32_t);
		uint32_t drawCount = 1;
		drawCountBuffer = device->Create(bufferCreateInfo, &drawCount);

		std::vector<DrawIndexedIndirectCommand> drawIndexedIndirectCmds{{static_cast<uint32_t>(indices.size()), 1, 0, 0, 0}};

		//create draw indexed indirect command buffer
		BufferCreateInfo drawIndexedIndirectCmdBufferCreateInfo{
			{},
			sizeof(DrawIndexedIndirectCommand) * drawIndexedIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndexedIndirectCmdBuffer = device->Create(drawIndexedIndirectCmdBufferCreateInfo, drawIndexedIndirectCmds.data());
	}
	void createIndexBuffer()
	{
		BufferCreateInfo indexBufferCreateInfo{
			{},
			sizeof(indices[0]) * indices.size(),
			BufferUsageFlagBits::INDEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		indexBuffer = device->Create(indexBufferCreateInfo, indices.data());
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