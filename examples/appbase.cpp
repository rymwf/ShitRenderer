/**
 * @file appBase.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "appbase.hpp"

#define CUBEMAP_WIDTH 1024

static const char *axisVertShaderName = "axis.vert.spv";
static const char *axisFragShaderName = "axis.frag.spv";

static const char *cubemapVertShaderName = "cubemap.vert.spv";
static const char *cubemapFragShaderName = "cubemap.frag.spv";

//static const char *cubemapImagePath = IMAGE_PATH "Ridgecrest_Road_Ref.hdr";
//static const char *cubemapImagePath = IMAGE_PATH "grace-new.hdr";
//static const char *cubemapImagePath = IMAGE_PATH "Mt-Washington-Gold-Room_Ref.hdr";
static const char *cubemapImagePath = IMAGE_PATH "Mt-Washington-Cave-Room_Ref.hdr";
//static const char *cubemapImagePath = IMAGE_PATH "Mt-Washington-Cave-Room_Bg.jpg";
//static const char *cubemapImagePath = IMAGE_PATH "3DTotal_free_sample_2_Bg.jpg";

static glm::vec3 ambientColor = glm::vec3(0.2);

void Node::Update()
{
	globalMatrix = (pParent ? pParent->globalMatrix : glm::mat4(1)) *
				   glm::mat4(glm::scale(glm::translate(glm::dmat4(1), translation) * glm::mat4_cast(rotation), scale));
	updated = true;
	for (auto &&child : children)
		child->Update();
}
void Node::AddChild(Node *pNode)
{
	children.emplace_back(pNode);
	pNode->pParent = this;
}
void Node::RemoveChild(Node *pNode)
{
	auto it = std::find_if(children.begin(), children.end(), [pNode](auto &&e) {
		return e == pNode;
	});
	if (it != children.end())
		children.erase(it);
}
Camera::Camera()
{
	eye.translation = glm::dvec3(0, 0, 5);
	AddChild(&eye);

	frustumDescription = {0.1, 0, PerspectiveProjectionDescription{45, 1}};
	UpdateFrustum();
}
Camera::~Camera()
{
}
void Camera::UpdateFrustum()
{
	frustumUpdated = true;
	std::visit(
		[&](auto &&arg) {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, PerspectiveProjectionDescription>)
			{
				if (frustumDescription.far == 0)
					frustumMatrix = glm::infinitePerspective(glm::radians(arg.fovy), arg.aspect, frustumDescription.near);
				else
					frustumMatrix = glm::perspective(glm::radians(arg.fovy), arg.aspect, frustumDescription.near, frustumDescription.far);
			}
			else if constexpr (std::is_same_v<T, OrthogonalProjectionDescription>)
			{
				frustumMatrix = glm::ortho(-arg.xmag / 2, arg.xmag / 2, -arg.ymag / 2, arg.ymag / 2, frustumDescription.near, frustumDescription.far);
			}
			frustumMatrix[1][1] *= -1;
		},
		frustumDescription.extraDesc);
}
glm::mat4 Camera::GetProjectionMatrix()
{
	return frustumMatrix * glm::mat4(glm::translate(glm::transpose(glm::mat4(glm::mat3(eye.globalMatrix))), -glm::vec3(eye.globalMatrix[3])));
}
Model *Scene::LoadModel(const char *pPath)
{
	auto model = std::make_unique<Model>(pPath);
	if (model->LoadSucceed())
	{
		models.emplace_back(std::move(model));
		return models.back().get();
	}
	else
	{
		LOG("model load failed");
		return nullptr;
	}
}
void Scene::Destroy(Node *pNode)
{
	for (auto &&e : pNode->children)
		Destroy(e);
	nodes.erase(std::find_if(nodes.begin(), nodes.end(), [pNode](auto &&e) {
		return e.get() == pNode;
	}));
}
void Scene::Destroy(Model *pModel)
{
	models.erase(std::find_if(models.begin(), models.end(), [pModel](auto &&e) {
		return e.get() == pModel;
	}));
}
//===============================================================================
AppBase::AppBase(uint32_t width, uint32_t height) : _width(width), _height(height)
{
	initRenderSystem();
	createSwapchain();
	createDepthResources();
	createColorResources();
	createRenderPasses();
	createFramebuffers();
	createSyncObjects();
}
void AppBase::initRenderSystem()
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
		{{80, 40},
		 {_width, _height}},
	};
	//		std::make_shared<std::function<void(const Event &)>>(std::bind(&AppBase::processBaseEvent, this, std::placeholders::_1))};
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

	//5
	QueueCreateInfo queueCreateInfo{
		graphicsQueueFamilyIndex->index,
		0,
	};
	graphicsQueue = device->Create(queueCreateInfo);

	transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index, presentQueueFamilyIndex.index});
	if (!transferQueueFamilyIndex.has_value())
		THROW("failed to find a transfer queue");
}
void AppBase::createSwapchain()
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
		{_width, _height},
		1,
		ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::TRANSFER_SRC_BIT,
		presentMode};
	window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
	while (swapchainCreateInfo.imageExtent.width == 0 && swapchainCreateInfo.imageExtent.height == 0)
	{
		window->WaitEvents();
		window->GetFramebufferSize(swapchainCreateInfo.imageExtent.width, swapchainCreateInfo.imageExtent.height);
	}
	//set imgui

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
void AppBase::createDepthResources()
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
void AppBase::createColorResources()
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
void AppBase::createRenderPasses()
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

	defaultRenderPass = device->Create(renderPassCreateInfo);
}
void AppBase::createFramebuffers()
{
	FramebufferCreateInfo framebufferCreateInfo{
		defaultRenderPass,
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
void AppBase::destroyWindowResouces()
{
	device->Destroy(colorImageView);
	device->Destroy(colorImage);
	device->Destroy(depthImageView);
	device->Destroy(depthImage);
	device->Destroy(defaultRenderPass);
	for (uint32_t i = 0; i < swapchainImages.size(); ++i)
	{
		device->Destroy(framebuffers[i]);
		device->Destroy(swapchainImageViews[i]);
		device->Destroy(swapchainImages[i]);
	}
	device->Destroy(swapchain);
}
void AppBase::recreateSwapchain()
{
	destroyWindowResouces();
	createSwapchain();
	createDepthResources();
	createColorResources();
	createRenderPasses();
	createFramebuffers();
}
void AppBase::createSyncObjects()
{
	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		imageAvailableSemaphores[i] = device->Create(SemaphoreCreateInfo{});
		renderFinishedSemaphores[i] = device->Create(SemaphoreCreateInfo{});
		inFlightFences[i] = device->Create(FenceCreateInfo{FenceCreateFlagBits::SIGNALED_BIT});
	}
}
void AppBase::createSampler()
{
	linearSampler = device->Create(SamplerCreateInfo{
		Filter::LINEAR,
		Filter::LINEAR,
		SamplerMipmapMode::LINEAR,
		SamplerWrapMode::CLAMP_TO_EDGE,
		SamplerWrapMode::CLAMP_TO_EDGE,
		SamplerWrapMode::CLAMP_TO_EDGE,
		0,
		false,
		1.,
		false,
		{},
		0.f,
		30.f,
	});
}
void AppBase::processBaseEvent(const Event &ev)
{
	static bool mouseFlagL;
	ImGuiIO &io = ImGui::GetIO();
	std::visit(overloaded{
				   [&](const KeyEvent &value) {
					   if (value.keyCode == KeyCode::KEY_ESCAPE)
						   ev.pWindow->Close();
					   if (value.keyCode == KeyCode::KEY_P && value.action == PressAction::DOWN)
						   startScreenshot = true;
				   },
				   [&](const CharEvent &value) {
					   io.AddInputCharacter(value.codepoint);
				   },
				   [](auto &&) {},
				   [&](const WindowResizeEvent &value) {
					   uint32_t width, height;
					   ev.pWindow->GetFramebufferSize(width, height);
					   io.DisplaySize.x = width;
					   io.DisplaySize.y = height;
				   },
				   [&](const MouseButtonEvent &value) {
					   mouseFlagL = io.MouseDown[0] = value.button == MouseButton::MOUSE_L && value.action == PressAction::DOWN;
					   io.MouseDown[1] = value.button == MouseButton::MOUSE_R && value.action == PressAction::DOWN;
					   io.MouseDown[1] = value.button == MouseButton::MOUSE_M && value.action == PressAction::DOWN;
				   },
				   [&](const MouseMoveEvent &value) {
					   //imgui
					   io.MousePos.x = value.xpos;
					   io.MousePos.y = value.ypos;
					   if (!io.WantCaptureMouse && static_cast<bool>(ev.modifier & EventModifierBits::BUTTONL))
					   {
						   //modify camera
						   static int pre_x, pre_y;
						   if (mouseFlagL)
						   {
							   pre_x = value.xpos;
							   pre_y = value.ypos;
							   mouseFlagL = false;
							   return;
						   }
						   double theta = double(value.ypos - pre_y) / 10.;
						   double phi = double(value.xpos - pre_x) / 10.;
						   pre_x = value.xpos;
						   pre_y = value.ypos;

						   //TODO: change camera
						   mainCamera->rotation = glm::dquat(glm::dvec3(0, glm::radians(-phi), 0)) * mainCamera->rotation *
												  glm::dquat(glm::dvec3(glm::radians(-theta), 0, 0));
						   //mainCamera->translation = glm::dvec3(glm::mat4_cast(mainCamera->rotation) * glm::dvec4(mainCamera->translation, 1));
						   mainCamera->Update();
					   }
				   },
				   [&](const MouseWheelEvent &value) {
					   //imgui
					   io.MouseWheelH += value.xoffset;
					   io.MouseWheel += value.yoffset;
					   if (!io.WantCaptureMouse)
					   {
						   mainCamera->eye.translation *= (1 + double(-value.yoffset) / 10.);
						   mainCamera->Update();
					   }
				   },
			   },
			   ev.value);

	processEvent(ev);
}
void AppBase::mainloop()
{
	while (window->PollEvents())
	{
		auto temptime = std::chrono::system_clock::now();
		frameTimeInterval_ms = std::chrono::duration<float, std::chrono::milliseconds::period>(temptime - curTime).count();
		curTime = temptime;
		FPS = static_cast<uint32_t>(1000 / frameTimeInterval_ms);

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
		}
		else if (ret != Result::SUCCESS)
		{
			THROW("failed to get next image");
		}
		//=======================================
		updateDefaultBuffers(imageIndex);
		std::vector<CommandBuffer *> tempCmdBuffers;
		drawFrame(imageIndex, tempCmdBuffers);
		drawUI(imageIndex);

		recordBackgroundSecondaryCommandBuffers(imageIndex);
		recordDefaultCommandBuffers(imageIndex, frameSecondaryCommandBuffers[imageIndex]);

		tempCmdBuffers.emplace_back(defaultCommandBuffers[imageIndex]);
		//====================
		inFlightFences[currentFrame]->Reset();

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphores[currentFrame]},
			 tempCmdBuffers,
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
			takeScreenshot(device, swapchainImages[imageIndex], ImageLayout::PRESENT_SRC);
			startScreenshot = false;
		}
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	presentQueue->WaitIdle();
}
void AppBase::run()
{
	//create rootnode
	rootNode = scene.CreateNode<Node>();

	//create main camera
	mainCamera = scene.CreateNode<Camera>(rootNode);
	mainCamera->frustumDescription = {0.1, 0,
									  PerspectiveProjectionDescription{
										  45.f,
										  float(swapchain->GetCreateInfoPtr()->imageExtent.width) / swapchain->GetCreateInfoPtr()->imageExtent.height}};
	mainCamera->UpdateFrustum();

	//create light
	directionalLight = scene.CreateNode<Light>(rootNode);
	directionalLight->translation = glm::dvec3(10);
	directionalLight->rotation = glm::dquat(glm::dvec3(1, 0, 0), glm::dvec3(1));

	rootNode->Update();

	createDefaultCommandPool();
	createDefaultCommandBuffers();

	createUBOFrameBuffers();
	createDefaultDescriptorSets();
	createSampler();

	prepareBackground();
	prepare();

	prepareImGui();

	//add event listener
	window->AddEventListener(std::make_shared<std::function<void(const Event &)>>(std::bind(&AppBase::processBaseEvent, this, std::placeholders::_1)));

	//example
	mainloop();
}
void AppBase::prepareBackground()
{
	prepareSkybox();
	prepareAxis();
}
void AppBase::recordBackgroundSecondaryCommandBuffers(uint32_t imageIndex)
{
	backgroundSecondaryCommandBuffers[imageIndex]->Begin(
		{CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT,
		 CommandBufferInheritanceInfo{defaultRenderPass, 0, framebuffers[imageIndex]}});

	Viewport viewport{0, 0, swapchain->GetCreateInfoPtr()->imageExtent.width, swapchain->GetCreateInfoPtr()->imageExtent.height, 0, 1};
	backgroundSecondaryCommandBuffers[imageIndex]->SetViewport(SetViewPortInfo{0, 1, &viewport});
	Rect2D scissor{{}, swapchain->GetCreateInfoPtr()->imageExtent};
	backgroundSecondaryCommandBuffers[imageIndex]->SetScissor(SetScissorInfo{0, 1, &scissor});

	//render axis
	backgroundSecondaryCommandBuffers[imageIndex]->BindDescriptorSets(
		BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			pipelineLayoutAxis,
			DESCRIPTORSET_ID_FRAME,
			1,
			&defaultDescriptorSetsUboFrame[imageIndex]});

	backgroundSecondaryCommandBuffers[imageIndex]->BindPipeline({PipelineBindPoint::GRAPHICS, pipelineAxis});
	backgroundSecondaryCommandBuffers[imageIndex]->DrawIndirect({drawIndirectCmdBufferAxis,
																 0,
																 1,
																 sizeof(DrawIndirectCommand)});

	//draw cubemap
	backgroundSecondaryCommandBuffers[imageIndex]->BindDescriptorSets(
		BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			skyboxPipelineLayout,
			DESCRIPTORSET_ID_FRAME,
			1,
			&defaultDescriptorSetsUboFrame[imageIndex]});
	backgroundSecondaryCommandBuffers[imageIndex]->BindDescriptorSets(
		BindDescriptorSetsInfo{
			PipelineBindPoint::GRAPHICS,
			skyboxPipelineLayout,
			1,
			1,
			&skyboxDescriptorSets[0]});
	backgroundSecondaryCommandBuffers[imageIndex]->BindPipeline({PipelineBindPoint::GRAPHICS, skyboxPipeline});
	backgroundSecondaryCommandBuffers[imageIndex]->DrawIndirect({skyboxIndirectDrawCmdBuffer,
																 0,
																 1,
																 sizeof(DrawIndirectCommand)});

	backgroundSecondaryCommandBuffers[imageIndex]->End();
}
void AppBase::createDefaultCommandPool()
{
	defaultLongLiveCommandPool = device->Create(CommandPoolCreateInfo{{}, graphicsQueueFamilyIndex->index});

	defaultShortLiveCommandPool = device->Create(
		CommandPoolCreateInfo{CommandPoolCreateFlagBits::TRANSIENT_BIT | CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
							  graphicsQueueFamilyIndex->index});
}
void AppBase::createDefaultCommandBuffers()
{
	defaultShortLiveCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{CommandBufferLevel::SECONDARY, static_cast<uint32_t>(swapchainImages.size())},
		backgroundSecondaryCommandBuffers);

	uint32_t count = static_cast<uint32_t>(swapchainImageViews.size());
	CommandBufferCreateInfo cmdBufferCreateInfo{CommandBufferLevel::PRIMARY, count};

	defaultShortLiveCommandPool->CreateCommandBuffers(cmdBufferCreateInfo, defaultCommandBuffers);
	frameSecondaryCommandBuffers.resize(count);
	for (uint32_t i = 0; i < count; ++i)
	{
		frameSecondaryCommandBuffers[i].emplace_back(backgroundSecondaryCommandBuffers[i]);
	}
}
void AppBase::recordDefaultCommandBuffers(uint32_t imageIndex, const std::vector<CommandBuffer *> &secondaryCommandBuffers)
{
	//record commnadbuffer
	std::vector<ClearValue> clearValues{
		std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
		std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
		ClearDepthStencilValue{1.f, 0},
	};

	defaultCommandBuffers[imageIndex]->Begin({CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
	//secondary commandBuffers
	defaultCommandBuffers[imageIndex]->BeginRenderPass(RenderPassBeginInfo{
		defaultRenderPass,
		framebuffers[imageIndex],
		Rect2D{
			{},
			swapchain->GetCreateInfoPtr()->imageExtent},
		static_cast<uint32_t>(clearValues.size()),
		clearValues.data(),
		SubpassContents::SECONDARY_COMMAND_BUFFERS});
	defaultCommandBuffers[imageIndex]->ExecuteSecondaryCommandBuffer(
		{static_cast<uint32_t>(secondaryCommandBuffers.size()),
		 secondaryCommandBuffers.data()});
	defaultCommandBuffers[imageIndex]->EndRenderPass();
	defaultCommandBuffers[imageIndex]->End();
}

void AppBase::createUBOFrameBuffers()
{
	BufferCreateInfo bufferCreateInfo{
		.size = sizeof(UBOFrame),
		.usage = BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
		.memoryPropertyFlags = MemoryPropertyFlagBits::HOST_COHERENT_BIT | MemoryPropertyFlagBits::HOST_VISIBLE_BIT};
	uboFrameBuffers.resize(swapchainImages.size());
	for (auto &&e : uboFrameBuffers)
		e = device->Create(bufferCreateInfo, nullptr);
}
void AppBase::updateDefaultBuffers(uint32_t imageIndex)
{
	static UBOFrame uboFrame{};
	static std::vector<bool> updated(swapchainImages.size(), true);
	if (mainCamera->updated || mainCamera->frustumUpdated)
	{
		uboFrame.ambientColor = ambientColor;
		uboFrame.PV = mainCamera->GetProjectionMatrix();
		uboFrame.eyePosition = glm::vec3(mainCamera->eye.globalMatrix[3]);
		updated.assign(swapchainImages.size(), true);
		mainCamera->updated = false;
		mainCamera->frustumUpdated = false;
	}
	if (directionalLight->updated)
	{
		uboFrame.light.pos = glm::vec3(directionalLight->globalMatrix[3]);
		uboFrame.light.direction = glm::mat3(directionalLight->globalMatrix) * glm::vec3(0, 0, -1);
		uboFrame.light.lim_r_min = 1.f;
		uboFrame.light.lim_r_max = 50.f;
		updated.assign(swapchainImages.size(), true);
		directionalLight->updated = false;
	}
	if (updated[imageIndex])
	{
		void *pData;
		auto size = sizeof(glm::mat4) + sizeof(glm::vec3);
		uboFrameBuffers[imageIndex]->MapMemory(0, size, &pData);
		memcpy(pData, &uboFrame, size);
		uboFrameBuffers[imageIndex]->UnMapMemory();
		updated[imageIndex] = false;
	}
}
void AppBase::createDefaultDescriptorSets()
{
	//descriptor sets
	std::vector<DescriptorSetLayoutBinding> PVBindings{
		DescriptorSetLayoutBinding{UNIFORM_BINDING_FRAME, DescriptorType::UNIFORM_BUFFER, 1, ShaderStageFlagBits::VERTEX_BIT | ShaderStageFlagBits::FRAGMENT_BIT}, //UBO PV
	};

	defaultDescriptorSetLayouts = {device->Create(DescriptorSetLayoutCreateInfo{PVBindings})};

	auto setCount = swapchainImages.size();
	std::vector<DescriptorPoolSize> poolSizes(PVBindings.size());
	std::vector<DescriptorSetLayout *> setLayouts(setCount, defaultDescriptorSetLayouts[0]);
	std::transform(PVBindings.begin(), PVBindings.end(), poolSizes.begin(), [](auto &&e) {
		return DescriptorPoolSize{e.descriptorType, e.descriptorCount};
	});
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		static_cast<uint32_t>(setCount),
		poolSizes};
	defaultDescriptorPool = device->Create(descriptorPoolCreateInfo);

	DescriptorSetAllocateInfo allocInfo{setLayouts};
	defaultDescriptorPool->Allocate(allocInfo, defaultDescriptorSetsUboFrame);

	//update descriptorsets
	std::vector<WriteDescriptorSet> writes(setCount);
	for (size_t i = 0; i < setCount; ++i)
	{
		writes[i] = WriteDescriptorSet{
			defaultDescriptorSetsUboFrame[i],
			UNIFORM_BINDING_FRAME,
			0,
			DescriptorType::UNIFORM_BUFFER,
			std::vector<DescriptorBufferInfo>{{uboFrameBuffers[i],
											   0,
											   sizeof(UBOFrame)}}};
	}
	device->UpdateDescriptorSets(writes, {});
}
void AppBase::prepareAxis()
{
	//=======================================================
	std::string axisVertShaderPath = buildShaderPath(axisVertShaderName, rendererVersion);
	std::string axisFragShaderPath = buildShaderPath(axisFragShaderName, rendererVersion);

	auto vertSource = readFile(axisVertShaderPath.c_str());
	auto fragSource = readFile(axisFragShaderPath.c_str());

	Shader *axisVertShader = device->Create(ShaderCreateInfo{vertSource.size(), vertSource.data()});
	Shader *axisFragShader = device->Create(ShaderCreateInfo{fragSource.size(), fragSource.data()});

	//create pipeline layout
	pipelineLayoutAxis = device->Create(PipelineLayoutCreateInfo{{defaultDescriptorSetLayouts[0]}});

	//create pipeline
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
	PipelineInputAssemblyStateCreateInfo inputAssemblyState{
		PrimitiveTopology::LINE_LIST,
	};
	PipelineViewportStateCreateInfo viewportStateCreateInfo{
		{{0, 0, static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width), static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height), 0, 1}},
		{{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent}}};
	PipelineTessellationStateCreateInfo tessellationState{};
	PipelineRasterizationStateCreateInfo rasterizationState{
		false,
		false,
		PolygonMode::LINE,
		CullMode::BACK,
		FrontFace::COUNTER_CLOCKWISE,
		false,
	};
	rasterizationState.lineWidth = 1.f;

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
		.depthCompareOp = CompareOp::LESS_OR_EQUAL,
	};

	PipelineColorBlendAttachmentState colorBlendAttachmentstate{
		false,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT};
	PipelineColorBlendStateCreateInfo colorBlendState{
		false,
		LogicOp::COPY,
		{colorBlendAttachmentstate},
	};
	PipelineDynamicStateCreateInfo dynamicStateInfo{
		{
			DynamicState::VIEWPORT,
			DynamicState::SCISSOR,
		}};

	VertexInputStateCreateInfo vertexInputStateCreateInfo{};
	pipelineAxis = device->Create(GraphicsPipelineCreateInfo{
		axisShaderStageCreateInfos,
		{},
		PipelineInputAssemblyStateCreateInfo{PrimitiveTopology::LINE_LIST},
		viewportStateCreateInfo,
		tessellationState,
		rasterizationState,
		multisampleState,
		depthStencilState,
		colorBlendState,
		dynamicStateInfo,
		pipelineLayoutAxis,
		defaultRenderPass,
		0});

	//axis draw command buffer
	std::vector<DrawIndirectCommand> drawIndirectCmds{{6, 1, 0, 0}};
	BufferCreateInfo bufferCreateInfo{
		{},
		sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
		BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	drawIndirectCmdBufferAxis = device->Create(bufferCreateInfo, drawIndirectCmds.data());
}
void AppBase::prepareSkybox()
{
	//cubemap rendering descriptors
	//descriptor sets
	std::vector<DescriptorSetLayoutBinding> texBindings{
		DescriptorSetLayoutBinding{0, DescriptorType::COMBINED_IMAGE_SAMPLER, 1, ShaderStageFlagBits::FRAGMENT_BIT}, //
	};

	std::vector<DescriptorSetLayout *> setLayouts = {
		device->Create(DescriptorSetLayoutCreateInfo{texBindings}),
	};
	//create pipeline layout
	skyboxPipelineLayout = device->Create(PipelineLayoutCreateInfo{{defaultDescriptorSetLayouts[0], setLayouts[0]}});

	//setup descriptor pool
	std::vector<DescriptorPoolSize> poolSizes{
		{texBindings[0].descriptorType, texBindings[0].descriptorCount},
	};
	DescriptorPoolCreateInfo descriptorPoolCreateInfo{
		1u,
		poolSizes};
	DescriptorPool *descriptorPool = device->Create(descriptorPoolCreateInfo);

	//allocate cubmap descriptor sets
	DescriptorSetAllocateInfo allocInfo = {{setLayouts[0]}};
	descriptorPool->Allocate(allocInfo, skyboxDescriptorSets);

	//===================================================
	//generate cubemap
	//load image
	int width, height, components;
	auto pixels = loadImage(cubemapImagePath, width, height, components, 4); //force load an alpha channel,even not exist

	//=========================================================
	//create cubemap
	ImageCreateInfo imageCreateInfo{
		.imageType = ImageType::TYPE_2D,
		.format = ShitFormat::RGBA32_SFLOAT, //TODO: check if format support storage image
		.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
		.mipLevels = 0,
		.arrayLayers = 1,
		.samples = SampleCountFlagBits::BIT_1,
		.tiling = ImageTiling::OPTIMAL,
		.usageFlags = ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::TRANSFER_SRC_BIT,
		.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
	};
	skyboxImage2D = device->Create(imageCreateInfo, pixels);
	freeImage(pixels);
	skyboxImage2D->GenerateMipmaps(Filter::LINEAR, ImageLayout::SHADER_READ_ONLY_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	//cubmap
	imageCreateInfo = {
		ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
		ImageType::TYPE_2D,
		ShitFormat::RGBA32_SFLOAT,
		{CUBEMAP_WIDTH, CUBEMAP_WIDTH, 1},
		0,
		6,
		SampleCountFlagBits::BIT_1,
		ImageTiling::OPTIMAL,
		ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	skyboxImageCube = device->Create(imageCreateInfo, nullptr);

	ImageViewCreateInfo imageViewCreateInfo = {
		skyboxImageCube,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA32_SFLOAT,
		{},
		{0, skyboxImageCube->GetCreateInfoPtr()->mipLevels, 0, 6}};
	skyboxImageViewCube = device->Create(imageViewCreateInfo);

	//=======================================================
	//equirectangular2cube shader
	convert2DToCubemap(
		device,
		skyboxImage2D,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		skyboxImageCube,
		ImageLayout::UNDEFINED,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	//update descriptor set
	std::vector<WriteDescriptorSet> writes;
	writes.emplace_back(
		WriteDescriptorSet{
			skyboxDescriptorSets[0],
			0,
			0,
			DescriptorType::COMBINED_IMAGE_SAMPLER,
			std::vector<DescriptorImageInfo>{{linearSampler,
											  skyboxImageViewCube,
											  ImageLayout::SHADER_READ_ONLY_OPTIMAL}}});
	device->UpdateDescriptorSets(writes, {});

	//=========================================================
	//create shaders

	std::string vertShaderPath = buildShaderPath(cubemapVertShaderName, rendererVersion);
	std::string fragShaderPath = buildShaderPath(cubemapFragShaderName, rendererVersion);

	auto vertSource = readFile(vertShaderPath.c_str());
	auto fragSource = readFile(fragShaderPath.c_str());

	Shader *cubemapVertShader = device->Create(ShaderCreateInfo{vertSource.size(), vertSource.data()});
	Shader *cubemapFragShader = device->Create(ShaderCreateInfo{fragSource.size(), fragSource.data()});

	//cubemap pipeline
	std::vector<PipelineShaderStageCreateInfo> cubemapShaderStageCreateInfos{
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::VERTEX_BIT,
			cubemapVertShader,
			"main",
		},
		PipelineShaderStageCreateInfo{
			ShaderStageFlagBits::FRAGMENT_BIT,
			cubemapFragShader,
			"main",
		},
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
		.depthCompareOp = CompareOp::LESS_OR_EQUAL,
	};

	PipelineColorBlendAttachmentState colorBlendAttachmentstate{
		false,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		BlendFactor::SRC_ALPHA,
		BlendFactor::ONE_MINUS_SRC_ALPHA,
		BlendOp::ADD,
		ColorComponentFlagBits::R_BIT | ColorComponentFlagBits::G_BIT | ColorComponentFlagBits::B_BIT | ColorComponentFlagBits::A_BIT};
	PipelineColorBlendStateCreateInfo colorBlendState{
		false,
		LogicOp::COPY,
		{colorBlendAttachmentstate},
	};
	PipelineDynamicStateCreateInfo dynamicStateInfo{
		{
			DynamicState::VIEWPORT,
			DynamicState::SCISSOR,
		}};
	VertexInputStateCreateInfo vertexInputStateCreateInfo{};
	skyboxPipeline = device->Create(GraphicsPipelineCreateInfo{
		cubemapShaderStageCreateInfos,
		{},
		PipelineInputAssemblyStateCreateInfo{PrimitiveTopology::TRIANGLE_LIST},
		viewportStateCreateInfo,
		tessellationState,
		rasterizationState,
		multisampleState,
		depthStencilState,
		colorBlendState,
		dynamicStateInfo,
		skyboxPipelineLayout,
		defaultRenderPass,
		0});
	//create draw command buffer
	std::vector<DrawIndirectCommand> drawIndirectCmds{{36, 1, 0, 0}};
	skyboxIndirectDrawCmdBuffer = device->Create(
		BufferCreateInfo{
			{},
			sizeof(DrawIndirectCommand) * drawIndirectCmds.size(),
			BufferUsageFlagBits::INDIRECT_BUFFER_BIT | BufferUsageFlagBits::TRANSFER_DST_BIT,
			MemoryPropertyFlagBits::DEVICE_LOCAL_BIT},
		drawIndirectCmds.data());
}
void AppBase::addSecondaryCommandBuffers(uint32_t imageIndex, const std::vector<CommandBuffer *> &commandBuffers)
{
	frameSecondaryCommandBuffers[imageIndex].insert(
		frameSecondaryCommandBuffers[imageIndex].end(),
		commandBuffers.begin(),
		commandBuffers.end());
}
void AppBase::removeSecondaryCommandBuffer(uint32_t imageIndex, const CommandBuffer *pCommandBuffer)
{
	frameSecondaryCommandBuffers[imageIndex].erase(std::find_if(
		frameSecondaryCommandBuffers[imageIndex].begin(),
		frameSecondaryCommandBuffers[imageIndex].end(),
		[pCommandBuffer](auto e) {
			return e == pCommandBuffer;
		}));
}
void AppBase::prepareImGui()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.DisplaySize.x = swapchain->GetCreateInfoPtr()->imageExtent.width;
	io.DisplaySize.y = swapchain->GetCreateInfoPtr()->imageExtent.height;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	defaultShortLiveCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{
			CommandBufferLevel::SECONDARY,
			static_cast<uint32_t>(swapchain->GetImageNum())},
		imguiCommandBuffers);
	//add imgui commandbuffers to the end
	for (uint32_t i = 0; i < swapchain->GetImageNum(); ++i)
	{
		addSecondaryCommandBuffers(i, {imguiCommandBuffers[i]});
	}
	ImGui_ImplShitRenderer_InitInfo imguiInitInfo{
		renderSystem,
		device,
		imguiCommandBuffers,
		swapchain->GetImageNum(),
		SAMPLE_COUNT};
	ImGui_ImplShitRenderer_Init(&imguiInitInfo, defaultRenderPass, 0);

	executeOneTimeCommands(device, QueueFlagBits::TRANSFER_BIT, 0, [](CommandBuffer *commandBuffer) {
		ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer);
	});
	ImGui_ImplShitRenderer_DestroyFontUploadObjects();
}
void AppBase::drawUI(uint32_t imageIndex)
{
	static bool show_demo_window = true;
	ImGui_ImplShitRenderer_NewFrame();
	ImGui::NewFrame();
	//=======================================

	static bool demoWindow = false;
	static bool windowSkybox = false;

	//main menu bar
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				window->Close();
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("GameObject"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Component"))
		{
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Window"))
		{
			if (ImGui::MenuItem("DemoWindow"))
			{
				demoWindow = true;
			}
			if (ImGui::MenuItem("skybox"))
			{
				windowSkybox = true;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Help"))
		{
			ImGui::EndMenu();
		}
		ImGui::EndMainMenuBar();
	}

	//==========================
	if (demoWindow)
		ImGui::ShowDemoWindow(&demoWindow);
	if (windowSkybox)
	{
		ImGui::Begin("skybox inspector", &windowSkybox);
		ImGui::Text("TODO");
		ImGui::End();
	}

	//============================
	drawImGui();
	//=======================================
	ImGui::Render();
	ImGui_ImplShitRenderer_RecordCommandBuffer(imageIndex, defaultRenderPass, 0, framebuffers[imageIndex]);
}