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

static const char *cubemapImagePath = IMAGE_PATH "Mt-Washington-Cave-Room_Bg.jpg";
//static const char *cubemapImagePath = IMAGE_PATH "3DTotal_free_sample_2_Bg.jpg";

static glm::vec3 ambientColor = glm::vec3(0.0);

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
	createSwapchains();
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
		{WindowCreateFlagBits::FIXED_SIZE},
		__FILE__,
		{{80, 40},
		 {_width, _height}},
		std::make_shared<std::function<void(const Event &)>>(std::bind(&AppBase::processBaseEvent, this, std::placeholders::_1))};
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
void AppBase::createSwapchains()
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

	renderPass = device->Create(renderPassCreateInfo);
}
void AppBase::createFramebuffers()
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
		false,
		{},
		0.f,
		30.f,
	});
}
void AppBase::processBaseEvent(const Event &ev)
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
				   [&](const WindowResizeEvent &value) {},
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
					   mainCamera->eye.translation *= (1 + double(-value.yoffset) / 10.);
					   mainCamera->Update();
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
			//recreateSwapchain();
		}
		else if (ret != Result::SUCCESS)
		{
			THROW("failed to get next image");
		}
		//=======================================
		updateDefaultBuffers(imageIndex);

		std::vector<CommandBuffer *> otherPrimaryCommandBuffers;
		drawFrame(imageIndex, otherPrimaryCommandBuffers);

		static std::vector<CommandBuffer *> commandBuffers;
		commandBuffers = {defaultCommandBuffers[imageIndex]};
		commandBuffers.insert(commandBuffers.end(), otherPrimaryCommandBuffers.begin(), otherPrimaryCommandBuffers.end());
		//====================
		inFlightFences[currentFrame]->Reset();

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphores[currentFrame]},
			 commandBuffers,
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
			//recreateSwapchain();
		}
		else if (res != Result::SUCCESS)
		{
			THROW("failed to present swapchain image");
		}
		if (startScreenshot)
		{
			takeScreenshot(device, swapchainImages[imageIndex], ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
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
	createUBOFrameBuffers();
	createDefaultDescriptorSets();
	createSampler();

	prepareBackground();
	prepare();

	createDefaultCommandBuffers();
	//example
	mainloop();
}
void AppBase::createDefaultCommandPool()
{
	CommandPoolCreateInfo commandPoolCreateInfo{
		{},
		graphicsQueueFamilyIndex->index};
	defaultCommandPool = device->Create(commandPoolCreateInfo);

	frameSecondaryCommandBuffers.resize(swapchainImages.size());

	defaultCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{CommandBufferLevel::SECONDARY, static_cast<uint32_t>(swapchainImages.size())},
		backgroundSecondaryCommandBuffers);
}
void AppBase::prepareBackground()
{
	prepareSkybox();
	createIrradianceMap();
	createPrefilteredEnvMap();

	prepareAxis();

	auto count = swapchainImages.size();
	backgroundSecondaryCommandBuffers.resize(count);
	for (size_t i = 0; i < count; ++i)
	{
		backgroundSecondaryCommandBuffers[i]->Begin({{}, CommandBufferInheritanceInfo{renderPass, 0, framebuffers[i]}});

		//render axis
		backgroundSecondaryCommandBuffers[i]->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				pipelineLayoutAxis,
				DESCRIPTORSET_ID_FRAME,
				1,
				&defaultDescriptorSetsUboFrame[i]});

		backgroundSecondaryCommandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, pipelineAxis});
		backgroundSecondaryCommandBuffers[i]->DrawIndirect({drawIndirectCmdBufferAxis,
															0,
															1,
															sizeof(DrawIndirectCommand)});

		//draw cubemap
		backgroundSecondaryCommandBuffers[i]->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				skyboxPipelineLayout,
				DESCRIPTORSET_ID_FRAME,
				1,
				&defaultDescriptorSetsUboFrame[i]});
		backgroundSecondaryCommandBuffers[i]->BindDescriptorSets(
			BindDescriptorSetsInfo{
				PipelineBindPoint::GRAPHICS,
				skyboxPipelineLayout,
				1,
				1,
				&skyboxDescriptorSets[0]});
		backgroundSecondaryCommandBuffers[i]->BindPipeline({PipelineBindPoint::GRAPHICS, skyboxPipeline});
		backgroundSecondaryCommandBuffers[i]->DrawIndirect({skyboxIndirectDrawCmdBuffer,
															0,
															1,
															sizeof(DrawIndirectCommand)});

		backgroundSecondaryCommandBuffers[i]->End();
		frameSecondaryCommandBuffers[i].emplace_back(backgroundSecondaryCommandBuffers[i]);
	}
}
void AppBase::createDefaultCommandBuffers()
{
	for (auto e : defaultCommandBuffers)
	{
		defaultCommandPool->DestroyCommandBuffer(e);
	}
	uint32_t count = static_cast<uint32_t>(swapchainImageViews.size());
	CommandBufferCreateInfo cmdBufferCreateInfo{CommandBufferLevel::PRIMARY, count};

	defaultCommandPool->CreateCommandBuffers(cmdBufferCreateInfo, defaultCommandBuffers);
	//record commnadbuffer
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

	for (uint32_t i = 0; i < swapchainImages.size(); ++i)
	{
		renderPassBeginInfo.pFramebuffer = framebuffers[i];
		defaultCommandBuffers[i]->Begin(cmdBufferBeginInfo);
		//secondary commandBuffers
		defaultCommandBuffers[i]->BeginRenderPass(RenderPassBeginInfo{
			renderPass,
			framebuffers[i],
			Rect2D{
				{},
				swapchain->GetCreateInfoPtr()->imageExtent},
			static_cast<uint32_t>(clearValues.size()),
			clearValues.data(),
			SubpassContents::SECONDARY_COMMAND_BUFFERS});
		defaultCommandBuffers[i]->ExecuteSecondaryCommandBuffer(
			{static_cast<uint32_t>(frameSecondaryCommandBuffers[i].size()),
			 frameSecondaryCommandBuffers[i].data()});
		defaultCommandBuffers[i]->EndRenderPass();
		defaultCommandBuffers[i]->End();
	}
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

	Shader *axisVertShader = device->Create(ShaderCreateInfo{readFile(axisVertShaderPath.c_str())});
	Shader *axisFragShader = device->Create(ShaderCreateInfo{readFile(axisFragShaderPath.c_str())});

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
		{},
		pipelineLayoutAxis,
		renderPass,
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
	if (!pixels)
		throw std::runtime_error("failed to load texture image!");

	//=========================================================
	//create cubemap
	ImageCreateInfo imageCreateInfo{
		.imageType = ImageType::TYPE_2D,
		.format = ShitFormat::RGBA8_UNORM, //TODO: check if format support storage image
		.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = SampleCountFlagBits::BIT_1,
		.tiling = ImageTiling::OPTIMAL,
		.usageFlags = ImageUsageFlagBits::SAMPLED_BIT,
		.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
		.initialLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
	};
	skyboxImage2D = device->Create(imageCreateInfo, pixels);
	freeImage(pixels);

	ImageViewCreateInfo imageViewCreateInfo{
		.pImage = skyboxImage2D,
		.viewType = ImageViewType::TYPE_2D,
		.format = ShitFormat::RGBA8_UNORM,
		.subresourceRange = {0, 1, 0, 1},
	};
	skyboxImageView2D = device->Create(imageViewCreateInfo);

	//cubmap
	imageCreateInfo = {
		ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
		ImageType::TYPE_2D,
		ShitFormat::RGBA8_UNORM,
		{CUBEMAP_WIDTH, CUBEMAP_WIDTH, 1},
		0,
		6,
		SampleCountFlagBits::BIT_1,
		ImageTiling::OPTIMAL,
		ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
		MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
	skyboxImageCube = device->Create(imageCreateInfo, nullptr);

	imageViewCreateInfo = {
		skyboxImageCube,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, 1, 0, 6}};
	skyboxImageViewCube = device->Create(imageViewCreateInfo);

	//=======================================================
	//equirectangular2cube shader
	convert2DToCubemap(
		device,
		skyboxImageView2D,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		skyboxImageViewCube,
		ImageLayout::UNDEFINED,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	//generate mipmap
	skyboxImageCube->GenerateMipmaps(Filter::LINEAR, ImageLayout::SHADER_READ_ONLY_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	//change skyboxImageViewCube to all levels
	device->Destroy(skyboxImageViewCube);
	imageViewCreateInfo.subresourceRange.levelCount = skyboxImageCube->GetCreateInfoPtr()->mipLevels;
	skyboxImageViewCube = device->Create(imageViewCreateInfo);

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

	Shader *cubemapVertShader = device->Create(ShaderCreateInfo{readFile(vertShaderPath.c_str())});
	Shader *cubemapFragShader = device->Create(ShaderCreateInfo{readFile(fragShaderPath.c_str())});

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
		{},
		skyboxPipelineLayout,
		renderPass,
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
void AppBase::createIrradianceMap()
{
	ImageCreateInfo imageCreateInfo{
		.flags = ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
		.imageType = ImageType::TYPE_2D,
		.format = ShitFormat::RGBA8_UNORM, //TODO: check if format support storage image
		.extent = {IRRADIANCE_MAP_WIDTH, IRRADIANCE_MAP_WIDTH, 1},
		.mipLevels = 0,
		.arrayLayers = 6,
		.samples = SampleCountFlagBits::BIT_1,
		.tiling = ImageTiling::OPTIMAL,
		.usageFlags = ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT,
		.memoryPropertyFlags = MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
	};

	irradianceImageCube = device->Create(imageCreateInfo, nullptr);

	ImageViewCreateInfo imageViewCreateInfo{
		irradianceImageCube,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, 1, 0, 6},
	};

	irradianceImageViewCube = device->Create(imageViewCreateInfo);
	generateIrradianceMap(
		device,
		skyboxImageViewCube,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		irradianceImageViewCube,
		ImageLayout::UNDEFINED,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL);
	device->Destroy(irradianceImageViewCube);

	irradianceImageCube->GenerateMipmaps(Filter::LINEAR, ImageLayout::SHADER_READ_ONLY_OPTIMAL, ImageLayout::SHADER_READ_ONLY_OPTIMAL);

	imageViewCreateInfo.subresourceRange.levelCount = irradianceImageCube->GetCreateInfoPtr()->mipLevels;
	irradianceImageViewCube = device->Create(imageViewCreateInfo);
}
void AppBase::createPrefilteredEnvMap()
{
	ImageCreateInfo imageCreateInfo = *skyboxImageCube->GetCreateInfoPtr();
	imageCreateInfo.usageFlags = ImageUsageFlagBits::SAMPLED_BIT | ImageUsageFlagBits::STORAGE_BIT;
	imageCreateInfo.initialLayout = ImageLayout::UNDEFINED;

	prefilteredEnvMap = device->Create(imageCreateInfo, nullptr);

	prefilteredEnvMapView = device->Create(ImageViewCreateInfo{
		prefilteredEnvMap,
		ImageViewType::TYPE_CUBE,
		ShitFormat::RGBA8_UNORM,
		{},
		{0, prefilteredEnvMap->GetCreateInfoPtr()->mipLevels, 0, 6},
	});
	generatePrefilteredEnvMap(
		device,
		skyboxImageCube,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL,
		prefilteredEnvMap,
		ImageLayout::UNDEFINED,
		ImageLayout::SHADER_READ_ONLY_OPTIMAL);
}