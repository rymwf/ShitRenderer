#include "common.hpp"
#include "model.hpp"

#define MODEL_SIZE 1.
#define PERSPECTIVE 1

uint32_t WIDTH = 800, HEIGHT = 600;

constexpr SampleCountFlagBits SAMPLE_COUNT = SampleCountFlagBits::BIT_4;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

int animationIndex = 0;
glm::vec3 ambientColor = glm::vec3(0.0);

const char *vertShaderName = "10.vert.spv";
const char *tescShaderName = "10.tesc.spv";
const char *teseShaderName = "10.tese.spv";
const char *fragShaderName = "10.frag.spv";

const char *axisVertShaderName = "axis.vert.spv";
const char *axisFragShaderName = "axis.frag.spv";

const char *testModelPath = ASSET_PATH "glTF-Sample-Models/2.0/Triangle/glTF/Triangle.gltf";
//const char *testModelPath = ASSET_PATH "glTF-Sample-Models/2.0/TwoSidedPlane/glTF/TwoSidedPlane.gltf";

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
	Shader *tescShader;
	Shader *teseShader;

	DescriptorPool *descriptorPool;

	RenderPass *renderPass;

	PipelineLayout *pipelineLayout;
	Pipeline *pipeline;
	std::vector<DescriptorSetLayout *> descriptorSetLayouts;
	std::vector<DescriptorSet *> descriptorSets;

	//==============
	//axis
	Shader *axisVertShader;
	Shader *axisFragShader;
	Pipeline *axisPipeline;

	Buffer *drawIndirectCmdBuffer;
	//=========

	std::vector<Buffer *> uboFrameBuffers;

	Image *depthImage;
	ImageView *depthImageView;

	Image *colorImage;
	ImageView *colorImageView;

	uint32_t FPS;
	float frameTimeInterval_ms;
	std::chrono::system_clock::time_point curTime;
	std::chrono::system_clock::time_point startTime;

	//model
	std::unique_ptr<Model> testModel;
	std::unique_ptr<Model> testModel2;

//camera
#if PERSPECTIVE
	Frustum frustum{{0.1, 0., PerspectiveProjectionDescription{glm::radians(45.f), 1}}};
#else
	Frustum frustum{{-10., 10., OrthogonalProjectionDescription{5., 5.}}};
#endif
	//Camera camera{glm::dvec3(0, -5, 0), glm::dvec3(0, 0, 1), glm::dvec3(0, 0, 1)};
	Camera camera{glm::dvec3(0, 0, 3), glm::dvec3(0, 0, 0), glm::dvec3(0, 1, 0)};

	bool startScreenshot{};

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
		createColorResources();

		createShaders();
		createDescriptorSets();
		createRenderPasses();
		createFramebuffers();
		createSyncObjects();
		createUBOPVBuffers();
		updateDescriptorSets();

		testModel = std::make_unique<Model>(testModelPath);
		if (testModel->LoadSucceed())
			DownloadModel(testModel.get());
		else
			testModel.reset();

		createDrawCommandBuffers();

		createPipeline();
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
					   [&](const KeyEvent &value) {
						   if (value.keyCode == KeyCode::KEY_ESCAPE)
							   ev.pWindow->Close();
						   if (value.keyCode == KeyCode::KEY_P && value.action == PressAction::DOWN)
							   startScreenshot = true;
					   },
					   [](auto &&) {},
					   [&](const WindowResizeEvent &value) {
						   if (value.height == 0)
							   return;
						   std::visit(
							   [&](auto &&e) {
								   using T = std::decay_t<decltype(e)>;
								   if constexpr (std::is_same_v<T, PerspectiveProjectionDescription>)
								   {
									   e = PerspectiveProjectionDescription{
										   glm::radians(45.), double(value.width) / value.height};
								   }
							   },
							   frustum.projectionDescription.extraDesc);
						   frustum.Update();
					   },
					   [&](const MouseButtonEvent &value) {
						   if (value.button == MouseButton::MOUSE_L)
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
							   static int pre_x, pre_y;
							   if (mouseFlagL)
							   {
								   pre_x = value.xpos;
								   pre_y = value.ypos;
								   mouseFlagL = false;
								   return;
							   }
							   double theta = double(value.ypos - pre_y) / 100.;
							   double phi = double(value.xpos - pre_x) / 100.;
							   pre_x = value.xpos;
							   pre_y = value.ypos;

							   auto dir = camera.eye - camera.center;
							   auto eye = glm::dmat3(glm::rotate(glm::rotate(glm::dmat4(1), -phi, camera.up), -theta, glm::cross(camera.up, dir))) * dir + camera.center;
							   if (glm::length(glm::cross((eye - camera.center), camera.up)) > 0.01)
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
					   [&](const DropEvent &value) {
						   LOG(value.paths[0]);
						   testModel2 = std::make_unique<Model>(value.paths[0]);
						   if (!testModel2->LoadSucceed())
							   testModel2.reset();
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
		device->Destroy(colorImage);
		device->Destroy(colorImageView);
		device->Destroy(depthImage);
		device->Destroy(depthImageView);
		for (auto &&e : swapchainImageViews)
			device->Destroy(e);
		device->Destroy(renderPass);
		for (auto &&e : framebuffers)
			device->Destroy(e);
		device->Destroy(pipeline);
		device->Destroy(axisPipeline);
		for (auto &&e : commandBuffers)
			commandPool->DestroyCommandBuffer(e);
		device->Destroy(swapchain);
	}
	void recreateSwapchain()
	{
		cleanupSwapchain();
		createSwapchains();
		createDepthResources();
		createColorResources();
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
		//		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		if (testModel2)
		{
			presentQueue->WaitIdle();
			//destroy old model
			testModel.reset();
			for (auto &&e : commandBuffers)
				commandPool->DestroyCommandBuffer(e);
			device->Destroy(pipeline);

			//download new model
			testModel = std::move(testModel2);

			DownloadModel(testModel.get());

			createPipeline();
			createCommandBuffers();
		}

		updateUBOPVBuffers(imageIndex);

		if (testModel->GetAnimationNum() > 0)
		{
			//update model animation
			static auto animationStartTime = curTime;
			static float cycleTime = -1; //ms
			auto elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(curTime - animationStartTime).count();
			testModel->UpdateAnimation(animationIndex, elapsedTime, imageIndex);
		}

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
		if (startScreenshot)
		{
			takeScreenshot(device, swapchainImages[imageIndex]);
			startScreenshot = false;
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void createShaders()
	{
		std::string vertShaderPath = buildShaderPath(vertShaderName, rendererVersion);
		std::string fragShaderPath = buildShaderPath(fragShaderName, rendererVersion);
		std::string tescShaderPath = buildShaderPath(tescShaderName, rendererVersion);
		std::string teseShaderPath = buildShaderPath(teseShaderName, rendererVersion);

		vertShader = device->Create(ShaderCreateInfo{readFile(vertShaderPath.c_str())});
		fragShader = device->Create(ShaderCreateInfo{readFile(fragShaderPath.c_str())});
		tescShader = device->Create(ShaderCreateInfo{readFile(tescShaderPath.c_str())});
		teseShader = device->Create(ShaderCreateInfo{readFile(teseShaderPath.c_str())});

		//-----------
		std::string axisVertShaderPath = buildShaderPath(axisVertShaderName, rendererVersion);
		std::string axisFragShaderPath = buildShaderPath(axisFragShaderName, rendererVersion);

		axisVertShader = device->Create(ShaderCreateInfo{readFile(axisVertShaderPath.c_str())});
		axisFragShader = device->Create(ShaderCreateInfo{readFile(axisFragShaderPath.c_str())});
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
			2, //min image count
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
		std::vector<DescriptorSetLayoutBinding> frameBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT}, //UBO PV
		};
		std::vector<DescriptorSetLayoutBinding> nodeBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_NODE, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT}, //UBO M
		};
		std::vector<DescriptorSetLayoutBinding> materialBindings{
			DescriptorSetLayoutBinding{TEXTURE_BINDING_ALBEDO, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},				//albedo
			DescriptorSetLayoutBinding{TEXTURE_BINDING_NORMAL, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},				//normal
			DescriptorSetLayoutBinding{TEXTURE_BINDING_METALLIC_ROUGHNESS, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT}, //metallic and roughness
			DescriptorSetLayoutBinding{TEXTURE_BINDING_OCCLUSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},			//occlusion
			DescriptorSetLayoutBinding{TEXTURE_BINDING_EMISSION, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},			//emission
			DescriptorSetLayoutBinding{TEXTURE_BINDING_TRANSPARENCY, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},		//transparency
			DescriptorSetLayoutBinding{UNIFORM_BINDING_MATERIAL, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::FRAGMENT_BIT | ShaderStageFlagBits::VERTEX_BIT},					//UBO material
		};
		std::vector<DescriptorSetLayoutBinding> jointMatrixBindings{
			DescriptorSetLayoutBinding{UNIFORM_BINDING_JOINTMATRIX, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT},
		};

		descriptorSetLayouts.resize(4);
		descriptorSetLayouts[DESCRIPTORSET_ID_FRAME] = (device->Create(DescriptorSetLayoutCreateInfo{frameBindings}));
		descriptorSetLayouts[DESCRIPTORSET_ID_NODE] = (device->Create(DescriptorSetLayoutCreateInfo{nodeBindings}));
		descriptorSetLayouts[DESCRIPTORSET_ID_MATERIAL] = (device->Create(DescriptorSetLayoutCreateInfo{materialBindings}));
		descriptorSetLayouts[DESCRIPTORSET_ID_JOINTMATRIX] = (device->Create(DescriptorSetLayoutCreateInfo{jointMatrixBindings}));

		//pipelineLayout
		pipelineLayout = device->Create(PipelineLayoutCreateInfo{descriptorSetLayouts});

		//frame binding descriptors
		std::vector<DescriptorSetLayout *> setLayouts(swapchainImages.size(), descriptorSetLayouts[0]);

		std::vector<DescriptorPoolSize> poolSizes(frameBindings.size());
		std::transform(frameBindings.begin(), frameBindings.end(), poolSizes.begin(), [](auto &&e) {
			return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
		});
		DescriptorPoolCreateInfo descriptorPoolCreateInfo{
			.maxSets = static_cast<uint32_t>(swapchainImageViews.size()),
			.poolSizes = poolSizes};
		descriptorPool = device->Create(descriptorPoolCreateInfo);

		DescriptorSetAllocateInfo allocInfo{setLayouts};
		descriptorPool->Allocate(allocInfo, descriptorSets);
	}
	void updateDescriptorSets()
	{
		std::vector<WriteDescriptorSet> writes;
		auto len = descriptorSets.size();
		while (len-- > 0)
		{
			writes.emplace_back(
				WriteDescriptorSet{
					descriptorSets[len],
					UNIFORM_BINDING_FRAME,
					0,
					DescriptorType::UNIFORM_BUFFER,
					std::vector<DescriptorBufferInfo>{{uboFrameBuffers[len],
													   0,
													   sizeof(UBOFrame)}}});
		}
		device->UpdateDescriptorSets(writes, {});
	}
	void createRenderPasses()
	{
		std::vector<AttachmentDescription> attachmentDescriptions{
			{swapchain->GetCreateInfoPtr()->format,
			 SAMPLE_COUNT,
			 AttachmentLoadOp::CLEAR,
			 AttachmentStoreOp::DONT_CARE,
			 AttachmentLoadOp::DONT_CARE,
			 AttachmentStoreOp::DONT_CARE,
			 ImageLayout::UNDEFINED, //inital layout
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
			{swapchain->GetCreateInfoPtr()->format,
			 SampleCountFlagBits::BIT_1,
			 AttachmentLoadOp::CLEAR,	   //depth
			 AttachmentStoreOp::DONT_CARE, //depth
			 AttachmentLoadOp::DONT_CARE,  //stencil
			 AttachmentStoreOp::DONT_CARE, //stencil
			 ImageLayout::UNDEFINED,	   //inital layout
			 ImageLayout::PRESENT_SRC},	   //final layout
			{ShitFormat::D24_UNORM_S8_UINT,
			 SAMPLE_COUNT,
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
		std::vector<AttachmentReference> resolveAttachments{
			{1,
			 ImageLayout::COLOR_ATTACHMENT_OPTIMAL}};
		AttachmentReference depthAttachment{
			2, //the index of attachment description
			ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		std::vector<SubpassDescription> subPasses{
			{PipelineBindPoint::GRAPHICS,
			 {},
			 colorAttachments,
			 resolveAttachments,
			 depthAttachment}};
		RenderPassCreateInfo renderPassCreateInfo{
			attachmentDescriptions,
			subPasses};

		renderPass = device->Create(renderPassCreateInfo);
	}
	void createPipeline()
	{
		uint32_t jointMatrixNum = testModel->GetJointMatrixMaxNum();
		std::vector<uint32_t> constantIDs{CONSTANT_ID_JOINTNUM};
		std::vector<uint32_t> constantValues{jointMatrixNum};

		std::vector<PipelineShaderStageCreateInfo>
			shaderStageCreateInfos{
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::VERTEX_BIT,
					vertShader,
					"main",
					{constantIDs,
					 constantValues}},
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::TESSELLATION_CONTROL_BIT,
					tescShader,
					"main",
				},
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::TESSELLATION_EVALUATION_BIT,
					teseShader,
					"main",
				},
				PipelineShaderStageCreateInfo{
					ShaderStageFlagBits::FRAGMENT_BIT,
					fragShader,
					"main",
				},
			};
		PipelineInputAssemblyStateCreateInfo inputAssemblyState{
			PrimitiveTopology::PATCH_LIST,
		};
		PipelineViewportStateCreateInfo viewportStateCreateInfo{
			{{0, 0, static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width), static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height), 0, 1}},
			{{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent}}};
		PipelineTessellationStateCreateInfo tessellationState{3};
		PipelineRasterizationStateCreateInfo rasterizationState{
			false,
			false,
			PolygonMode::LINE,
			CullMode::BACK,
			FrontFace::COUNTER_CLOCKWISE,
			false,
			0.f,
			0.f,
			0.f,
			2.f};

		PipelineMultisampleStateCreateInfo multisampleState{
			SAMPLE_COUNT,
			true,	 //sample shading
			1.f,	 //min sample shading
			nullptr, //mask
			false,	 //alpha to coverage
			false	 //alpha to one
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
			{colorBlendAttachmentstate},
		};

		VertexInputStateCreateInfo vertexInputStateCreateInfo{
			{
				{LOCATION_POSITION},
				{LOCATION_NORMAL},
				{LOCATION_TANGENT},
				{LOCATION_TEXCOORD0},
				{LOCATION_TEXCOORD1},
				{LOCATION_COLOR0},
				{LOCATION_JOINTS0},
				{LOCATION_WEIGHTS0},
				{LOCATION_INSTANCE_COLOR_FACTOR, sizeof(InstanceAttribute), 1},
			},
			{

				{LOCATION_POSITION, LOCATION_POSITION, 3, DataType::FLOAT, false, 0},
				{LOCATION_NORMAL, LOCATION_NORMAL, 3, DataType::FLOAT, false, 0},
				{LOCATION_TANGENT, LOCATION_TANGENT, 4, DataType::FLOAT, false, 0},
				{LOCATION_TEXCOORD0, LOCATION_TEXCOORD0, 2, DataType::FLOAT, false, 0},
				{LOCATION_TEXCOORD1, LOCATION_TEXCOORD1, 2, DataType::FLOAT, false, 0},
				{LOCATION_COLOR0, LOCATION_COLOR0, 4, DataType::FLOAT, false, 0},
				{LOCATION_JOINTS0, LOCATION_JOINTS0, 4, DataType::FLOAT, false, 0},
				{LOCATION_WEIGHTS0, LOCATION_WEIGHTS0, 4, DataType::FLOAT, false, 0},
				{LOCATION_INSTANCE_COLOR_FACTOR, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 0},
				{LOCATION_INSTANCE_MATRIX + 0, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 16},
				{LOCATION_INSTANCE_MATRIX + 1, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 32},
				{LOCATION_INSTANCE_MATRIX + 2, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 48},
				{LOCATION_INSTANCE_MATRIX + 3, LOCATION_INSTANCE_COLOR_FACTOR, 4, DataType::FLOAT, false, 64},
			}};

		GraphicsPipelineCreateInfo pipelineCreateInfo{
			shaderStageCreateInfos,
			*testModel->GetVertexInputStateCreateInfoPtr(),
			inputAssemblyState,
			viewportStateCreateInfo,
			tessellationState,
			rasterizationState,
			multisampleState,
			depthStencilState,
			colorBlendState,
			{},
			pipelineLayout,
			renderPass,
			0};

		pipeline = device->Create(pipelineCreateInfo);

		//======================================================
		//axis pipeline
		rasterizationState.lineWidth = 1.f;
		std::vector<PipelineShaderStageCreateInfo> axisShaderStageCreateInfos{
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::VERTEX_BIT,
				axisVertShader,
				"main",
			},
			PipelineShaderStageCreateInfo{
				ShaderStageFlagBits::FRAGMENT_BIT,
				axisFragShader,
				"main",
			},
		};
		axisPipeline = device->Create(GraphicsPipelineCreateInfo{
			axisShaderStageCreateInfos,
			{},
			PipelineInputAssemblyStateCreateInfo{PrimitiveTopology::LINE_LIST},
			viewportStateCreateInfo,
			tessellationState,
			rasterizationState,
			multisampleState,
			depthStencilState,
			colorBlendState,
			{},
			pipelineLayout,
			renderPass,
			0});
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
			framebufferCreateInfo.attachments = {colorImageView, swapchainImageViews[count], depthImageView};
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
			std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
			std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
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

			commandBuffers[i]->BindDescriptorSets(
				BindDescriptorSetsInfo{
					PipelineBindPoint::GRAPHICS,
					pipelineLayout,
					DESCRIPTORSET_ID_FRAME,
					1,
					&descriptorSets[DESCRIPTORSET_ID_FRAME]});

			commandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, axisPipeline});
			DrawIndirectInfo drawCmdInfo{
				drawIndirectCmdBuffer,
				0,
				1,
				sizeof(DrawIndirectCommand)};
			commandBuffers[i]->DrawIndirect(drawCmdInfo);
			if (testModel)
			{
				commandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, pipeline});
				testModel->DrawModel(device, commandBuffers[i], i);
			}
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
	}
	void createUBOPVBuffers()
	{
		UBOFrame uboFrame{{},
						  glm::vec3(camera.eye),
						  {
							  glm::vec3(10, 10, 10),
							  glm::vec4(1),
							  glm::vec4(1),
							  glm::vec2(1, 100),
							  glm::vec3(-1),
						  },
						  ambientColor};
		//move model to origin
		BufferCreateInfo bufferCreateInfo{
			.size = sizeof(UBOFrame),
			.usage = BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
			.memoryPropertyFlags = MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
		uboFrameBuffers.resize(swapchainImages.size());
		for (auto &&e : uboFrameBuffers)
			e = device->Create(bufferCreateInfo, &uboFrame);
	}
	void updateUBOPVBuffers(size_t index)
	{
		static std::vector<bool> updated(swapchainImages.size(), true);
		static UBOFrame uboFrame;
		if (frustum.isUpdated || camera.isUpdated)
		{
			uboFrame = {frustum.projectionMatrix * camera.viewMatrix, camera.eye};
			for (auto &&e : updated)
				e = true;
			frustum.isUpdated = false;
			camera.isUpdated = false;
		}
		if (updated[index])
		{
			void *pData;
			auto size = sizeof(glm::mat4) + sizeof(glm::vec3);
			uboFrameBuffers[index]->MapMemory(0, size, &pData);
			memcpy(pData, &uboFrame, size);
			uboFrameBuffers[index]->UnMapMemory();
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
			SAMPLE_COUNT,
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
	void createDrawCommandBuffers()
	{
		std::vector<DrawIndirectCommand> drawIndirectCmds{{6, 1, 0, 0}};
		BufferCreateInfo bufferCreateInfo{
			{},
			sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
		drawIndirectCmdBuffer = device->Create(bufferCreateInfo, drawIndirectCmds.data());
	}
	void createColorResources()
	{
		ImageCreateInfo createInfo{
			{},
			ImageType::TYPE_2D,
			swapchain->GetCreateInfoPtr()->format,
			{swapchain->GetCreateInfoPtr()->imageExtent.width, swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
			1,
			1,
			SAMPLE_COUNT,
			ImageTiling::OPTIMAL,
			ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		};
		colorImage = device->Create(createInfo, nullptr);
		ImageViewCreateInfo imageViewCreateInfo{
			colorImage,
			ImageViewType::TYPE_2D,
			swapchain->GetCreateInfoPtr()->format,
			{},
			{0, 1, 0, 1}};
		colorImageView = device->Create(imageViewCreateInfo);
	}
	void DownloadModel(Model *pModel)
	{
		pModel->DownloadModel(device, pipelineLayout, swapchainImages.size());

		auto &&modelCenter = pModel->GetModelBoundingVolumePtr()->box.aabb.center;
		auto a = pModel->GetModelBoundingVolumePtr()->box.aabb.maxValue - modelCenter;
		auto scaleFactor = MODEL_SIZE / (std::max)((std::max)(a.x, a.y), a.z);
		auto intanceAttribute = InstanceAttribute{glm::vec4(1)};
		auto trans = glm::dvec3(modelCenter.x, a.y, modelCenter.z) * scaleFactor;
		//auto trans = glm::dvec3(0, 0, 0);
		intanceAttribute.matrix = glm::translate(glm::scale(glm::translate(glm::dmat4(1), trans), glm::dvec3(scaleFactor)), -modelCenter);
		pModel->CreateImageInstanceAttributeBuffers(device, {intanceAttribute}, true);

		camera.center = trans;
		camera.eye.x = camera.center.x;
		camera.eye.y = camera.center.y;
		camera.Update();
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