#include "common.hpp"

#include <stb_image.h>

uint32_t WIDTH = 800, HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const char *vertShaderName = "05.vert.spv";
const char *fragShaderName = "05.frag.spv";
const char *testImagePath = IMAGE_PATH "Lenna_test.jpg";

std::string vertShaderPath;
std::string fragShaderPath;

std::vector<DrawIndexedIndirectCommand> drawIndexedIndirectCmds{{4, 2, 0, 0, 0}};

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;
	static VertexBindingDescription getVertexBindingDescription(uint32_t binding)
	{
		return {
			binding,
			sizeof(Vertex),
			0,
		};
	}
	static std::vector<VertexAttributeDescription> getVertexAttributeDescription(uint32_t startLocation, uint32_t binding)
	{
		return {
			{startLocation + 0,
			 binding,
			 3,
			 DataType::FLOAT,
			 false,
			 offsetof(Vertex, pos)},
			{startLocation + 1,
			 binding,
			 3,
			 DataType::FLOAT,
			 false,
			 offsetof(Vertex, color)},
			{startLocation + 2,
			 binding,
			 2,
			 DataType::FLOAT,
			 false,
			 offsetof(Vertex, texCoord)},
		};
	}
	static uint32_t getLocationCount()
	{
		return 3;
	}
};

std::vector<Vertex> vertices{
	{{-1, -1, 0}, {1, 0, 0}, {0, 0}},
	{{-1, 1, 0}, {0, 0, 1}, {0, 1}},
	{{1, -1, 0}, {0, 1, 0}, {1, 0}},
	{{1, 1, 0}, {1, 1, 0}, {1, 1}},
};
std::vector<uint16_t> indices{0, 1, 2, 3};

std::vector<InstanceAttribute> instanceAttributes{
	{glm::vec4(1), glm::rotate(glm::mat4(1), glm::radians(-15.f), glm::vec3(0, 1, 0))},
	{glm::vec4(1), glm::rotate(glm::mat4(1), glm::radians(15.f), glm::vec3(0, 1, 0))},
	//{glm::rotate(glm::translate(glm::mat4(1), glm::vec3(-.5f, 0, 0)), glm::radians(-15.f), glm::vec3(0, 1, 0))},
	//{glm::rotate(glm::translate(glm::mat4(1), glm::vec3(.5f, 0, 0)), glm::radians(15.f), glm::vec3(0, 1, 0))},
	//	{glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, -1, 0, 0, 1)},
	//	{glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1)},
};

struct alignas(16) UBO_MVP
{
	glm::mat4 M;
	glm::mat4 PV;
};

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

	Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];

	Shader *vertShader;
	Shader *fragShader;

	DescriptorPool *descriptorPool;
	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	PipelineLayout *pipelineLayout;
	RenderPass *renderPass;
	Pipeline *pipeline;

	Buffer *drawCountBuffer;
	Buffer *drawIndexedIndirectCmdBuffer;
	Buffer *indexBuffer;
	Buffer *vertexBuffer;
	Buffer *instanceBuffer;

	Image *testImage;
	ImageView *testImageView;
	Sampler *sampler;

	Image *depthImage;
	ImageView *depthImageView;

	std::vector<Buffer *> uboMVPBuffers;

	uint32_t FPS;
	float frameTimeInterval_ms;
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;

public:
	void initRenderSystem()
	{
		startTime = std::chrono::system_clock::now();
		RenderSystemCreateInfo renderSystemCreateInfo{
			.version = rendererVersion,
			.flags = RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT,
		};

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

		createSwapchains();
		createDepthResources();

		createShaders();
		createDescriptorSets();
		createRenderPasses();
		createPipeline();
		createFramebuffers();
		createSyncObjects();

		createDrawCommandBuffers();
		createIndexBuffer();
		createVertexBuffer();
		createInstanceBuffer();
		createImages();
		createSamplers();
		createUBOMVPBuffer();

		updateDescriptorSets();

		createCommandBuffers();

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
			auto temptime = std::chrono::system_clock::now();
			frameTimeInterval_ms = std::chrono::duration<float, std::chrono::milliseconds::period>(temptime - curTime).count();
			curTime = temptime;
			FPS = static_cast<uint32_t>(1000 / frameTimeInterval_ms);
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
		device->Destroy(depthImage);
		device->Destroy(depthImageView);
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
		createDepthResources();
		createRenderPasses();
		createFramebuffers();
		createPipeline();
		createCommandBuffers();
	}
	void drawFrame()
	{
		static uint32_t currentFrame{};
		inFlightFences[currentFrame]->WaitFor(UINT64_MAX);

		uint32_t imageIndex{};
		GetNextImageInfo nextImageInfo{
			UINT64_MAX,
			imageAvailableSemaphores[currentFrame]};
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
		//========================================
		updateUBOMVPBuffer(imageIndex);
		//=================================
		inFlightFences[currentFrame]->Reset();

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphores[currentFrame]},
			 {commandBuffers[imageIndex]},
			 {renderFinishedSemaphores[currentFrame]}}};
		graphicsQueue->Submit(submitInfos, inFlightFences[currentFrame]);

		PresentInfo presentInfo{
			{renderFinishedSemaphores[currentFrame]},
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
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
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
	void createDescriptorSets()
	{
		std::vector<DescriptorSetLayoutBinding> bindings{
			DescriptorSetLayoutBinding{
				.binding = 0,
				.descriptorType = DescriptorType::COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = ShaderStageFlagBits::FRAGMENT_BIT,
			},
			DescriptorSetLayoutBinding{
				.binding = 1,
				.descriptorType = DescriptorType::UNIFORM_BUFFER,
				.descriptorCount = 1,
				.stageFlags = ShaderStageFlagBits::VERTEX_BIT,
			},
		};
		descriptorSetLayouts.resize(swapchainImages.size(), device->Create(DescriptorSetLayoutCreateInfo{bindings}));

		std::vector<DescriptorPoolSize> poolSizes(bindings.size());
		std::transform(bindings.begin(), bindings.end(), poolSizes.begin(), [](auto &&e) {
			return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
		});
		DescriptorPoolCreateInfo descriptorPoolCreateInfo{
			.maxSets = static_cast<uint32_t>(swapchainImageViews.size()),
			.poolSizes = poolSizes};
		descriptorPool = device->Create(descriptorPoolCreateInfo);

		DescriptorSetAllocateInfo allocInfo{descriptorSetLayouts};
		descriptorPool->Allocate(allocInfo, descriptorSets);
	}
	void updateDescriptorSets()
	{
		std::vector<DescriptorImageInfo> imagesInfo{
			DescriptorImageInfo{
				.pSampler = sampler,
				.pImageView = testImageView,
				.imageLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL},
		};
		std::vector<DescriptorBufferInfo> buffersInfo{
			{nullptr, 0, sizeof(UBO_MVP)},
		};
		std::vector<WriteDescriptorSet> writes{
			WriteDescriptorSet{
				nullptr, //pDstset
				0,		 //dst binding
				0,		 //dstArrayElement
				DescriptorType::COMBINED_IMAGE_SAMPLER,
				imagesInfo,
			},
			WriteDescriptorSet{
				nullptr, //pDstset
				1,		 //dst binding
				0,		 //dstArrayElement
				DescriptorType::UNIFORM_BUFFER,
				buffersInfo,
			},
		};
		for (size_t i = 0, n = descriptorSets.size(); i < n; ++i)
		{
			for (auto &&a : writes)
				a.pDstSet = descriptorSets[i];
			buffersInfo[0].pBuffer = uboMVPBuffers[i];
			writes[1].values = buffersInfo;
			device->UpdateDescriptorSets(writes, {});
		}
	}
	void createRenderPasses()
	{
		std::vector<AttachmentDescription> attachmentDescriptions{
			{swapchain->GetCreateInfoPtr()->format,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,	   //depth
			 AttachmentStoreOp::DONT_CARE, //depth
			 AttachmentLoadOp::DONT_CARE,  //stencil
			 AttachmentStoreOp::DONT_CARE, //stencil
			 ImageLayout::UNDEFINED,	   //inital layout
			 ImageLayout::PRESENT_SRC},	   //final layout
			{ShitFormat::D24_UNORM_S8_UINT,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,
			 AttachmentStoreOp::DONT_CARE,
			 AttachmentLoadOp::DONT_CARE,
			 AttachmentStoreOp::DONT_CARE,
			 ImageLayout::UNDEFINED,
			 ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
		};
		std::vector<AttachmentReference> colorAttachments{
			{0, //the index of attachment description
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
		};
		AttachmentReference depthAttachment{
			1, //the index of attachment description
			ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		std::vector<SubpassDescription> subPasses{
			SubpassDescription{
				.pipelineBindPoint = PipelineBindPoint::GRAPHICS,
				.colorAttachments = colorAttachments,
				.depthStencilAttachment = depthAttachment,
			},
		};

		RenderPassCreateInfo renderPassCreateInfo{
			attachmentDescriptions,
			subPasses};

		renderPass = device->Create(renderPassCreateInfo);
	}
	void createPipeline()
	{
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{descriptorSetLayouts});
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
		auto vertexBindingDesc = Vertex::getVertexBindingDescription(0);
		auto vertexAttributeDesc = Vertex::getVertexAttributeDescription(0, 0);
		auto instanceBindingDesc = InstanceAttribute::getVertexBindingDescription(1);
		auto instanceAttributeDesc = InstanceAttribute::getVertexAttributeDescription(3, 1);

		VertexInputStateCreateInfo vertexInputState{
			{std::move(vertexBindingDesc), std::move(instanceBindingDesc)},
			std::move(vertexAttributeDesc),
		};
		vertexInputState.vertexAttributeDescriptions.insert(vertexInputState.vertexAttributeDescriptions.end(),
															instanceAttributeDesc.begin(), instanceAttributeDesc.end());
		PipelineInputAssemblyStateCreateInfo inputAssemblyState{
			PrimitiveTopology::TRIANGLE_STRIP,
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
		PipelineDepthStencilStateCreateInfo depthStencilState{
			.depthTestEnable = true,
			.depthWriteEnable = true,
			.depthCompareOp = CompareOp::LESS,
		};

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
			{},											//attachments
			swapchain->GetCreateInfoPtr()->imageExtent, //extent2D
			1											//layers
		};
		auto count = swapchainImageViews.size();
		framebuffers.resize(count);
		while (count-- > 0)
		{
			framebufferCreateInfo.attachments = {swapchainImageViews[count], depthImageView};
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

		std::vector<ClearValue> clearValues{
			std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f},
			ClearDepthStencilValue{1.f, 0},
		};

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
			std::vector<Buffer *> buffers{vertexBuffer, instanceBuffer};
			std::vector<uint64_t> offsets{0, 0};
			commandBuffers[i]->BindVertexBuffer(
				BindVertexBufferInfo{
					0,
					static_cast<uint32_t>(buffers.size()),
					buffers.data(),
					offsets.data()});

			BindDescriptorSetsInfo info{
				PipelineBindPoint::GRAPHICS,
				pipelineLayout,
				0,
				1,
				&descriptorSets[i]};
			commandBuffers[i]->BindDescriptorSets(info);

			//draw index
			commandBuffers[i]->BindIndexBuffer(BindIndexBufferInfo{
				indexBuffer,
				0,
				IndexType::UINT16});
			commandBuffers[i]->DrawIndexedIndirect(
				DrawIndirectInfo{
					drawIndexedIndirectCmdBuffer,
					0,
					1,
					sizeof(DrawIndexedIndirectCommand)});

			commandBuffers[i]->EndRenderPass();
			commandBuffers[i]->End();
		}
	}

	void createSyncObjects()
	{
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			imageAvailableSemaphores[i] = device->Create(SemaphoreCreateInfo{});
			renderFinishedSemaphores[i] = device->Create(SemaphoreCreateInfo{});
			inFlightFences[i] = device->Create(FenceCreateInfo{FenceCreateFlagBits::SIGNALED_BIT});
		}
		//inFlightImages.resize(swapchainImages.size());
	}
	void createDrawCommandBuffers()
	{
		//create draw indirect command buffer
		BufferCreateInfo bufferCreateInfo{
			{},
			sizeof(uint32_t),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		uint32_t drawCount = 1;
		drawCountBuffer = device->Create(bufferCreateInfo, &drawCount);

		//create draw indexed indirect command buffer
		BufferCreateInfo drawIndexedIndirectCmdBufferCreateInfo{
			{},
			sizeof(DrawIndexedIndirectCommand) * drawIndexedIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndexedIndirectCmdBuffer = device->Create(drawIndexedIndirectCmdBufferCreateInfo, drawIndexedIndirectCmds.data());
	}
	void createVertexBuffer()
	{
		BufferCreateInfo createInfo{
			{},
			sizeof(vertices[0]) * vertices.size(),
			BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

		vertexBuffer = device->Create(createInfo, vertices.data());
	}
	void createInstanceBuffer()
	{
		BufferCreateInfo createInfo{
			{},
			sizeof(instanceAttributes[0]) * instanceAttributes.size(),
			BufferUsageFlagBits::VERTEX_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		instanceBuffer = device->Create(createInfo, instanceAttributes.data());
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
	void createImages()
	{
		int width, height, components;
		auto pixels = stbi_load(testImagePath, &width, &height, &components, 4); //force load an alpha channel,even not exist
		if (!pixels)
			throw std::runtime_error("failed to load texture image!");
		//mipmapLevels = static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) + 1);

		ImageCreateInfo imageCreateInfo{
			.imageType = ImageType::TYPE_2D,
			.format = ShitFormat::RGBA8_SRGB,
			.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = SampleCountFlagBits::BIT_1,
			.tiling = ImageTiling::OPTIMAL,
			.usageFlags = ImageUsageFlagBits::TRANSFER_DST_BIT | ImageUsageFlagBits::SAMPLED_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
			.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL};

		testImage = device->Create(imageCreateInfo, pixels);

		stbi_image_free(pixels);

		ImageViewCreateInfo imageViewCreateInfo{
			.pImage = testImage,
			.viewType = ImageViewType::TYPE_2D,
			.format = ShitFormat::RGBA8_SRGB,
			.subresourceRange = {0, 1, 0, 1},
		};
		testImageView = device->Create(imageViewCreateInfo);
	}
	void createSamplers()
	{
		SamplerCreateInfo samplerCreateInfo{
			.magFilter = Filter::NEAREST,
			.minFilter = Filter::NEAREST,
			.mipmapMode = SamplerMipmapMode::NEAREST,
			.wrapModeU = SamplerWrapMode::REPEAT,
			.wrapModeV = SamplerWrapMode::REPEAT,
			.wrapModeW = SamplerWrapMode::REPEAT,
		};
		sampler = device->Create(samplerCreateInfo);
	}
	void createUBOMVPBuffer()
	{
		UBO_MVP uboMVP{
			.M = glm::mat4(1.f),
			.PV = glm::perspective(
					  glm::radians(45.f),
					  float(swapchain->GetCreateInfoPtr()->imageExtent.width) / float(swapchain->GetCreateInfoPtr()->imageExtent.height),
					  1.f, 10.f) *
				  glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0), glm::vec3(0, 1, 0)),
		};
		BufferCreateInfo bufferCreateInfo{
			.size = sizeof(UBO_MVP),
			.usage = BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
		uboMVPBuffers.resize(swapchainImages.size());
		for (auto &&e : uboMVPBuffers)
			e = device->Create(bufferCreateInfo, &uboMVP);
	}
	void updateUBOMVPBuffer([[maybe_unused]] size_t index)
	{
		float time = std::chrono::duration<float, std::chrono::seconds::period>(curTime - startTime).count();
		UBO_MVP uboMVP{
			//.M = glm::rotate(glm::mat4(1), time * glm::radians(30.f), glm::vec3(0, 1, 0)),
			.M = glm::translate(glm::mat4(1), glm::vec3(0, 0, std::sin(time))),
			.PV = glm::perspective(
					  glm::radians(45.f),
					  float(swapchain->GetCreateInfoPtr()->imageExtent.width) / float(swapchain->GetCreateInfoPtr()->imageExtent.height),
					  1.f, 10.f) *
				  glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0), glm::vec3(0, 1, 0)),
		};
		void *pData;
		uboMVPBuffers[index]->MapMemory(0, sizeof(UBO_MVP), &pData);
		memcpy(pData, &uboMVP, sizeof(UBO_MVP));
		uboMVPBuffers[index]->UnMapMemory();
	}
	void createDepthResources()
	{
		ImageCreateInfo depthImageCreateInfo{
			{}, //create flags
			ImageType::TYPE_2D,
			ShitFormat::D24_UNORM_S8_UINT,
			{swapchain->GetCreateInfoPtr()->imageExtent.width,
			 swapchain->GetCreateInfoPtr()->imageExtent.height,
			 1},
			1, //mipmap levels,
			1, //array layers
			SampleCountFlagBits::BIT_1,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		depthImage = device->Create(depthImageCreateInfo, nullptr);

		ImageViewCreateInfo depthImageViewCreateInfo{
			depthImage,
			ImageViewType::TYPE_2D,
			ShitFormat::D24_UNORM_S8_UINT,
			{},			 //component mapping
			{0, 1, 0, 1} //subresouce range
		};
		depthImageView = device->Create(depthImageViewCreateInfo);
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