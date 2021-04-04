#include "common.hpp"

uint32_t WIDTH = 800, HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const char *vertShaderName = "08.vert.spv";
const char *fragShaderName = "08.frag.spv";

const char *testModelPath = ASSET_PATH "glTF-Sample-Models/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf";

std::string vertShaderPath;
std::string fragShaderPath;

struct alignas(16) UBO_PV
{
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


	std::vector<Buffer*> uboPVBuffers;

	Sampler *sampler;

	Image *depthImage;
	ImageView *depthImageView;

	uint32_t FPS;
	float frameTimeInterval_ms;
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;


	//model
	std::unique_ptr<Model> testModel;
	std::vector<Buffer *> testModelBuffers;


	//camera
	Frustum frustum{PerspectiveProjectionDescription{1, 10, glm::radians(45.f), 1}};
	Camera camera{glm::dvec3(0, 5, 0), glm::dvec3(0), glm::dvec3(0, 0, 1)};

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
			__FILE__,
			{{SHIT_DEFAULT_WINDOW_X, SHIT_DEFAULT_WINDOW_Y},
			 {SHIT_DEFAULT_WINDOW_WIDTH, SHIT_DEFAULT_WINDOW_HEIGHT}},
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

		loadModels();

		createShaders();
		createDescriptorSets();
		createRenderPasses();
		createPipeline();
		createFramebuffers();

		createSamplers();
		createSyncObjects();
		createUBOPVBuffers();

		updateDescriptorSets();

		createCommandBuffers();

		transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index, presentQueueFamilyIndex.index});
		if (!transferQueueFamilyIndex.has_value())
			THROW("failed to find a transfer queue");

	}
	/**
	 * @brief process window event, do not write render code here
	 * 
	 * @param ev 
	 */
	void ProcessEvent(const Event &ev)
	{
		static bool mouseFlagL;
		std::visit(overloaded{
					   [&ev](const KeyEvent &value) {
						   if (value.keyCode == KeyCode::KEY_ESCAPE)
							   ev.pWindow->Close();
					   },
					   [](auto &&) {},
					   [&](const WindowResizeEvent &value) {
						   if (value.height == 0)
							   return;
						   frustum.projectionDescription = PerspectiveProjectionDescription{
							   .1, 10., glm::radians(45.), double(value.width) / value.height};
						   frustum.Update();
					   },
					   [&](const MouseButtonEvent &value) {
						   if (value.button == MouseButton::MOUSE_L )
						   {
							   if (value.action == PressAction::DOWN)
							   {
								   mouseFlagL = true;
							   }
						   }
					   },
					   [&](const MouseMoveEvent &value) {
						   if (static_cast<bool>(ev.modifier & EventModifierBits::BUTTONL))
						   {
							   static int pre_x,pre_y;
							   if(mouseFlagL)
							   {
								   pre_x = value.xpos;
								   pre_y = value.ypos;
								   mouseFlagL = false;
								   return;
							   }
							   double theta = double(value.ypos-pre_y) / 100.;
							   double phi = double(value.xpos-pre_x) / 100.;
							   pre_x=value.xpos;
							   pre_y=value.ypos;

							   auto dir = camera.eye - camera.center;
							   auto eye = glm::dmat3(glm::rotate(glm::rotate(glm::dmat4(1), -phi, camera.up), theta, glm::cross(camera.up, dir))) * dir + camera.center;
							   if (std::abs(glm::normalize(eye - camera.center).z) < 0.99)
							   {
								   camera.eye = eye;
								   camera.Update();
							   }
						   }
					   },
					   [&](const MouseWheelEvent &value) {
						   camera.eye += (camera.eye - camera.center) * double(-value.yoffset) / 10.;
						   camera.Update();
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
		updateUBOPVBuffers(imageIndex);
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
			DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //albedo
			DescriptorSetLayoutBinding{1, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //normal
			DescriptorSetLayoutBinding{2, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //metallic and roughness
			DescriptorSetLayoutBinding{3, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //occlusion
			DescriptorSetLayoutBinding{4, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //emission
			DescriptorSetLayoutBinding{5, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //transparency
			DescriptorSetLayoutBinding{12, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},			 //UBO M
			DescriptorSetLayoutBinding{13, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},			 //UBO PV
			DescriptorSetLayoutBinding{14, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT},		 //UBO material
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
		std::vector<WriteDescriptorSet> writes;
		auto len = descriptorSets.size();
		while (len-- > 0)
		{
			writes.clear();
			writes.emplace_back(
				WriteDescriptorSet{
					descriptorSets[len],
					UNIFORM_BINDING_PV,
					0,
					DescriptorType::UNIFORM_BUFFER,
					std::vector<DescriptorBufferInfo>{{uboPVBuffers[len],
													   0,
													   sizeof(glm::mat4)}}});
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
			CullMode::NONE,
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
			*testModel->GetVertexInputStateCreateInfoPtr(),
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

			testModel->DrawModel(device, commandBuffers[i], descriptorSets[i], pipelineLayout);

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
	void createUBOPVBuffers()
	{
		BufferCreateInfo bufferCreateInfo{
			.size = sizeof(UBO_PV),
			.usage = BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
		uboPVBuffers.resize(swapchainImages.size());
		for (auto &&e : uboPVBuffers)
			e = device->Create(bufferCreateInfo, nullptr);
	}
	void updateUBOPVBuffers(size_t index)
	{
		static std::vector<bool> updated(swapchainImages.size(), true);
		static UBO_PV uboPV;
		if (frustum.isUpdated || camera.isUpdated)
		{
			uboPV = {frustum.projectionMatrix * camera.viewMatrix};
			for (auto &&e : updated)
				e = true;
			frustum.isUpdated = false;
			camera.isUpdated = false;
		}
		if (updated[index])
		{
			void *pData;
			uboPVBuffers[index]->MapBuffer(0, sizeof(UBO_PV), &pData);
			memcpy(pData, &uboPV, sizeof(UBO_PV));
			uboPVBuffers[index]->UnMapBuffer();
			updated[index] = false;
		}
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

	void loadModels()
	{
		testModel = std::make_unique<Model>(testModelPath);
		testModel->DownloadModel(device);
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