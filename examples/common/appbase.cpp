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
#include "gui.hpp"

AppBase *g_App;

//===============================================================================
//
AppBase::AppBase(uint32_t width, uint32_t height) : windowWidth(width), windowHeight(height)
{
	initRenderSystem();
	createSwapchain();
	createDepthResources();
	createColorResources();
	createRenderPasses();
	createFramebuffers();
	createSyncObjects();

	g_App = this;
}
void AppBase::initRenderSystem()
{
	startTime = std::chrono::system_clock::now();
	RenderSystemCreateInfo renderSystemCreateInfo{
		.version = g_RendererVersion,
		.flags = RenderSystemCreateFlagBits::SHIT_CONTEXT_DEBUG_BIT,
	};

	renderSystem = LoadRenderSystem(renderSystemCreateInfo);
	//1. create window
	WindowCreateInfo windowCreateInfo{
		{},
		__FILE__,
		{{80, 40},
		 {windowWidth, windowHeight}},
	};
	//		std::make_shared<std::function<void(const Event &)>>(std::bind(&AppBase::processBaseEvent, this, std::placeholders::_1))};
	window = renderSystem->CreateRenderWindow(windowCreateInfo);
	//1.5 choose phyiscal device
	//2. create device of a physical device
	DeviceCreateInfo deviceCreateInfo{};
	if ((g_RendererVersion & RendererVersion::TypeBitmask) == RendererVersion::GL)
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

	//transferQueueFamilyIndex = device->GetQueueFamilyIndexByFlag(QueueFlagBits::TRANSFER_BIT, {graphicsQueueFamilyIndex->index});
	//if (!transferQueueFamilyIndex.has_value())
	//	THROW("failed to find a transfer queue");
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
		{windowWidth, windowHeight},
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
void AppBase::processEvent(const Event &ev)
{
	static bool mouseLPressed;
	static bool mouseMPressed;
	ImGuiIO &io = ImGui::GetIO();
	std::visit(overloaded{
				   [&](const KeyEvent &value) {
					   if (value.keyCode == KeyCode::KEY_ESCAPE)
						   ev.pWindow->Close();
					   startScreenshot = value.keyCode == KeyCode::KEY_P && value.action == PressAction::DOWN;
					   io.KeyCtrl = value.keyCode == KeyCode::KEY_CONTROL && value.action == PressAction::DOWN;
					   io.KeyShift = value.keyCode == KeyCode::KEY_SHIFT && value.action == PressAction::DOWN;
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

					   auto pCamera = scene->GetMainCamera();
					   char *p = reinterpret_cast<char *>(&pCamera->frustumDescription.extraDesc);
					   if (pCamera->cameraType == CameraType::PERSPECTIVE)
					   {
						   double aspect = float(width) / height;
						   memcpy(p + sizeof(double) * 3, &aspect, sizeof(double));
					   }
					   else if (pCamera->cameraType == CameraType::ORTHO)
					   {
						   double w = width, h = height;
						   memcpy(p + sizeof(double) * 2, &w, sizeof(double));
						   memcpy(p + sizeof(double) * 3, &h, sizeof(double));
					   }
					   pCamera->UpdateFrustum();
				   },
				   [&](const MouseButtonEvent &value) {
					   mouseLPressed = io.MouseDown[0] = value.button == MouseButton::MOUSE_L && value.action == PressAction::DOWN;
					   io.MouseDown[1] = value.button == MouseButton::MOUSE_R && value.action == PressAction::DOWN;
					   if (mouseMPressed && value.button == MouseButton::MOUSE_M && value.action == PressAction::UP)
					   {
						   scene->GetMainCamera()->translation = {};
						   scene->GetMainCamera()->Update();
					   }
					   mouseMPressed = io.MouseDown[2] = value.button == MouseButton::MOUSE_M && value.action == PressAction::DOWN;
				   },
				   [&](const MouseMoveEvent &value) {
					   //imgui
					   io.MousePos.x = value.xpos;
					   io.MousePos.y = value.ypos;
					   if (!io.WantCaptureMouse)
					   {
						   if (static_cast<bool>(ev.modifier & EventModifierBits::BUTTONL))
						   {
							   auto mainCamera = scene->GetMainCamera();
							   //modify camera
							   static int pre_x, pre_y;
							   if (mouseLPressed)
							   {
								   //init pre mouse pos
								   pre_x = value.xpos;
								   pre_y = value.ypos;
								   mouseLPressed = false;
								   return;
							   }

							   double theta = double(value.ypos - pre_y) / 10.;
							   double phi = double(value.xpos - pre_x) / 10.;
							   pre_x = value.xpos;
							   pre_y = value.ypos;

							   //TODO: change camera
							   mainCamera->rotation = glm::quat(glm::vec3(0, glm::radians(-phi), 0)) * scene->GetMainCamera()->rotation *
													  glm::quat(glm::vec3(glm::radians(-theta), 0, 0));
							   //mainCamera->translation = glm::dvec3(glm::mat4_cast(mainCamera->rotation) * glm::dvec4(mainCamera->translation, 1));
							   mainCamera->Update();
						   }
						   else if (static_cast<bool>(ev.modifier & EventModifierBits::BUTTONM))
						   {
							   auto mainCamera = scene->GetMainCamera();
							   //modify camera
							   static int pre_x, pre_y;
							   if (mouseMPressed)
							   {
								   //init pre mouse pos
								   pre_x = value.xpos;
								   pre_y = value.ypos;
								   mouseMPressed= false;
								   return;
							   }
							   auto r = glm::length(mainCamera->eye.translation);
							   double theta = r * double(value.ypos - pre_y) / 1000.;
							   double phi = r * double(value.xpos - pre_x) / 1000.;

							   mainCamera->translation = glm::mat3(mainCamera->globalMatrix) * glm::vec3(-phi, theta, 0.);
							   mainCamera->Update();
						   }
					   }
				   },
				   [&](const MouseWheelEvent &value) {
					   //imgui
					   io.MouseWheelH += value.xoffset;
					   io.MouseWheel += value.yoffset;
					   if (!io.WantCaptureMouse)
					   {
						   scene->GetMainCamera()->eye.translation *= (1 + double(-value.yoffset) / 10.);
						   scene->GetMainCamera()->Update();
					   }
				   },
			   },
			   ev.value);
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
		//update gui first
		gui->update(imageIndex);
		//update scene
		scene->Update(imageIndex, framebuffers[imageIndex]);

		draw(imageIndex);

		//render skybox and axes
		//====================
		inFlightFences[currentFrame]->Reset();

		std::vector<SubmitInfo> submitInfos{
			{{imageAvailableSemaphores[currentFrame]},
			 {defaultPrimaryCommandBuffers[imageIndex]},
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

void AppBase::createDefaultCommandBuffers()
{
	defaultCommandPool = device->Create(CommandPoolCreateInfo{{}, graphicsQueueFamilyIndex->index});
	defaultResetableCommandPool = device->Create(CommandPoolCreateInfo{CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT, graphicsQueueFamilyIndex->index});
	defaultResetableCommandPool->CreateCommandBuffers(
		CommandBufferCreateInfo{
			CommandBufferLevel::PRIMARY,
			static_cast<uint32_t>(swapchain->GetImageNum())},
		defaultPrimaryCommandBuffers);
}
void AppBase::draw(uint32_t imageIndex)
{
	//record commnadbuffer
	std::vector<ClearValue> clearValues{
		std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
		std::array<float, 4>{ambientColor.x, ambientColor.y, ambientColor.z, 1.f},
		ClearDepthStencilValue{1.f, 0},
	};
	defaultPrimaryCommandBuffers[imageIndex]->Begin(CommandBufferBeginInfo{CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
	defaultPrimaryCommandBuffers[imageIndex]->BeginRenderPass(RenderPassBeginInfo{
		defaultRenderPass,
		framebuffers[imageIndex],
		Rect2D{
			{},
			swapchain->GetCreateInfoPtr()->imageExtent},
		static_cast<uint32_t>(clearValues.size()),
		clearValues.data(),
		SubpassContents::SECONDARY_COMMAND_BUFFERS});


	auto sceneCommandBuffer = scene->GetCommandBuffer(imageIndex);
	//render scene first
	defaultPrimaryCommandBuffers[imageIndex]->ExecuteSecondaryCommandBuffer({1, &sceneCommandBuffer});

	//render imgui at last
	defaultPrimaryCommandBuffers[imageIndex]->ExecuteSecondaryCommandBuffer({1, &gui->getCommandBuffer(imageIndex)});
	defaultPrimaryCommandBuffers[imageIndex]->EndRenderPass();
	defaultPrimaryCommandBuffers[imageIndex]->End();
}
