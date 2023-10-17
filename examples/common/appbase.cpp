#include "appbase.h"

AppBase *g_App;

static void window_size_callback(GLFWwindow *window, int width, int height) {}
static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    Input::onFramebufferResize(width, height);
}
static void window_content_scale_callback(GLFWwindow *window, float xscale, float yscale) {
    // set_interface_scale(xscale, yscale);
}
static void window_pos_callback(GLFWwindow *window, int xpos, int ypos) {}
static void window_iconify_callback(GLFWwindow *window, int iconified) {
    if (iconified) {
        // The window was iconified
    } else {
        // The window was restored
    }
}
static void window_maximize_callback(GLFWwindow *window, int maximized) {
    if (maximized) {
        // The window was maximized
    } else {
        // The window was restored
    }
}
static void window_focus_callback(GLFWwindow *window, int focused) {
    if (focused) {
        // The window gained input focus
    } else {
        // The window lost input focus
    }
}

void window_refresh_callback(GLFWwindow *window) {
    // draw_editor_ui(window);
    // glfwSwapBuffers(window);
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    Input::onKeyboard(key, scancode, action, mods);
}
static void character_callback(GLFWwindow *window, unsigned int codepoint) {}
static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) { Input::onMouseMove(xpos, ypos); }
static void cursor_enter_callback(GLFWwindow *window, int entered) {
    if (entered) {
        // The cursor entered the content area of the window
    } else {
        // The cursor left the content area of the window
    }
}
static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
    Input::onMouseButton(button, action, mods);
}
static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { Input::onScroll(xoffset, yoffset); }
static void joystick_callback(int jid, int event) {
    if (event == GLFW_CONNECTED) {
        // The joystick was connected
    } else if (event == GLFW_DISCONNECTED) {
        // The joystick was disconnected
    }
}
static void drop_callback(GLFWwindow *window, int count, const char **paths) {
    // int i;
    // for (i = 0;  i < count;  i++)
    //     handle_dropped_file(paths[i]);
}

static Shit::SurfacePixelFormat chooseSwapchainFormat(const std::vector<Shit::SurfacePixelFormat> &candidates,
                                                      Shit::Surface *pSurface) {
    std::vector<Shit::SurfacePixelFormat> supportedFormats;
    pSurface->GetPixelFormats(supportedFormats);
    LOG("supported formats:");
    for (auto &&e : supportedFormats) {
        LOG_VAR(static_cast<int>(e.format));
        LOG_VAR(static_cast<int>(e.colorSpace));
        for (auto &&a : candidates) {
            if (e.format == a.format && e.colorSpace == a.colorSpace) return e;
        }
    }
    return supportedFormats[0];
}
static Shit::PresentMode choosePresentMode(const std::vector<Shit::PresentMode> &candidates, Shit::Surface *pSurface) {
    std::vector<Shit::PresentMode> modes;
    pSurface->GetPresentModes(modes);
    for (auto &&a : candidates) {
        for (auto &&e : modes) {
            if (static_cast<int>(e) == static_cast<int>(a)) return e;
        }
    }
    return modes[0];
}

//============================================================
AppBase::AppBase() { g_App = this; }
AppBase::~AppBase() { glfwDestroyWindow(_window); }
void AppBase::getFramebufferSize(int &width, int &height) { glfwGetFramebufferSize(_window, &width, &height); }
void AppBase::init(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle,
                   Shit::SampleCountFlagBits sampleCount) {
    _sampleCount = sampleCount;
    _windowTitle = windowTitle;
    _rendererVersion = rendererVersion;

    initRenderSystem();
    createSwapchain();
    createQueues();
    createCommandPool();
    createSyncObjects();
    createDescriptorPool();
    createDescriptorSetLayouts();
    initManagers();

    createDefaultRenderPass();
    createDefaultRenderTarget();
    initDefaultCommandBuffer();
}
void AppBase::initManagers() {
    _imageManager = std::make_unique<ImageManager>();
    _shaderManager = std::make_unique<ShaderManager>();
    _textureManager = std::make_unique<TextureManager>();
    _materialDataBlockManager = std::make_unique<MaterialDataBlockManager>();
    _modelManager = std::make_unique<ModelManager>();

    _shaderManager->registerResourceLoader<ShaderSpvLoader>(ResourceManager::DEFAULT_LOADER_NAME, _device);
}
void AppBase::initRenderSystem() {
    // create window
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    _window = glfwCreateWindow(1280, 720, __FILE__, NULL, NULL);

    if (!_window) {
        glfwTerminate();
        throw std::runtime_error("failed to create window");
    }
    glfwSetWindowUserPointer(_window, this);

    glfwSetWindowSizeCallback(_window, window_size_callback);
    glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);
    glfwSetWindowContentScaleCallback(_window, window_content_scale_callback);
    glfwSetWindowPosCallback(_window, window_pos_callback);
    glfwSetWindowIconifyCallback(_window, window_iconify_callback);
    glfwSetWindowMaximizeCallback(_window, window_maximize_callback);

    glfwSetWindowFocusCallback(_window, window_focus_callback);
    glfwSetWindowRefreshCallback(_window, window_refresh_callback);

    // glfwSetMonitorCallback(monitor_callback);

    glfwSetKeyCallback(_window, key_callback);
    glfwSetCharCallback(_window, character_callback);
    glfwSetCursorPosCallback(_window, cursor_position_callback);
    glfwSetCursorEnterCallback(_window, cursor_enter_callback);
    glfwSetMouseButtonCallback(_window, mouse_button_callback);
    glfwSetScrollCallback(_window, scroll_callback);
    glfwSetJoystickCallback(joystick_callback);
    glfwSetDropCallback(_window, drop_callback);

    Input::framebufferResizeSignal.Connect(
        std::bind(&AppBase::onFramebufferResize, this, std::placeholders::_1, std::placeholders::_2));
    Input::mouseButtonSignal.Connect(
        std::bind(&AppBase::onMouseButton, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    Input::mouseMoveSignal.Connect(
        std::bind(&AppBase::onMouseMove, this, std::placeholders::_1, std::placeholders::_2));
    Input::scrollSignal.Connect(std::bind(&AppBase::onScroll, this, std::placeholders::_1, std::placeholders::_2));
    Input::keyboardSignal.Connect(std::bind(&AppBase::onKeyboard, this, std::placeholders::_1, std::placeholders::_2,
                                            std::placeholders::_3, std::placeholders::_4));

    glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);

    //=====================
    _renderSystem = LoadRenderSystem(Shit::RenderSystemCreateInfo{
        _rendererVersion,
#ifndef NDEBUG
        Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT,
#endif
    });
    g_RendererVersion = _renderSystem->GetCreateInfo()->version;

#ifdef _WIN32
    _surface = _renderSystem->CreateSurface(Shit::SurfaceCreateInfoWin32{glfwGetWin32Window(_window)});
#endif

    // 1.5 choose phyiscal device
    // 2. create device of a physical device
    _device = _renderSystem->CreateDevice();
}
void AppBase::createSwapchain() {
    auto swapchainFormat = chooseSwapchainFormat(
        {
            //{Shit::Format::B8G8R8A8_UNORM, Shit::ColorSpace::SRGB_NONLINEAR},
            {Shit::Format::B8G8R8A8_SRGB, Shit::ColorSpace::SRGB_NONLINEAR},
        },
        _surface);
    LOG_VAR(static_cast<int>(swapchainFormat.format));
    LOG_VAR(static_cast<int>(swapchainFormat.colorSpace));

    auto presentMode = choosePresentMode({Shit::PresentMode::MAILBOX, Shit::PresentMode::FIFO},
                                         //{Shit::PresentMode::FIFO},
                                         _surface);
    LOG("selected present mode:", static_cast<int>(presentMode));

    Shit::SurfaceCapabilities caps;
    _surface->GetCapabilities(caps);
    auto imageCount = (std::min)(caps.minImageCount + 1, caps.maxImageCount);

    _swapchain = _surface->Create(Shit::SwapchainCreateInfo{_device,
                                                            imageCount,  // min image count
                                                            swapchainFormat.format,
                                                            swapchainFormat.colorSpace,
                                                            {(uint32_t)_framebufferWidth, (uint32_t)_framebufferHeight},
                                                            1,
                                                            Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT |
                                                                Shit::ImageUsageFlagBits::TRANSFER_SRC_BIT |
                                                                Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                                                            presentMode});
}
void AppBase::recreateSwapchain() {
    for (auto e : _defaultRenderTarget.imageViews) _device->Destroy(e);
    _defaultRenderTarget.imageViews.clear();
    for (auto e : _defaultRenderTarget.images) _device->Destroy(e);
    _defaultRenderTarget.images.clear();
    for (auto &&e : _defaultRenderTarget.frameBuffers) _device->Destroy(e);
    _defaultRenderTarget.frameBuffers.clear();

    int width, height;
    getFramebufferSize(width, height);
    _swapchain->Resize(width, height);

    createDefaultRenderTarget();
    _swapchainRecreateSignal();
    _needRecreateSwapchain = false;
    _needUpdateDefaultCommandBuffer = true;
}
void AppBase::createDefaultRenderPass() {
    Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
    auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                       Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

    // create renderpass
    std::vector<Shit::AttachmentDescription> attachmentDesc{
        // color
        {_swapchain->GetCreateInfoPtr()->format, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
         Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
         Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        // depth
        {depthFormat, _sampleCount, Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
         Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE, Shit::ImageLayout::UNDEFINED,
         Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
    };
    if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
        attachmentDesc[0].finalLayout = Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        // resolve attachment
        attachmentDesc.emplace_back(_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1,
                                    Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
                                    Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
                                    Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
    }
    Shit::AttachmentReference colorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
    Shit::AttachmentReference depthAttachment{1, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    Shit::AttachmentReference resolveAttachment{2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

    Shit::SubpassDescription subpassDesc[]{Shit::PipelineBindPoint::GRAPHICS,
                                           0,
                                           0,
                                           1,
                                           &colorAttachment,
                                           _sampleCount == Shit::SampleCountFlagBits::BIT_1 ? 0 : &resolveAttachment,
                                           &depthAttachment};

    _defaultRenderTarget.renderPass = _device->Create(
        Shit::RenderPassCreateInfo{(uint32_t)attachmentDesc.size(), attachmentDesc.data(), 1, subpassDesc});

    setDefaultRenderPassClearValue();
}
void AppBase::setDefaultRenderPassClearValue() {
    Shit::ClearColorValueFloat clearColor = {0.0, 0.0, 0.0, 1};
    Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
    _defaultRenderTarget.clearValues = {clearColor, clearDepthStencil};
    if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
        _defaultRenderTarget.clearValues.emplace_back(clearColor);
    }
}
void AppBase::createDefaultRenderTarget() {
    // create framebuffer
    // color
    Shit::ImageCreateInfo colorImageCI{
        {},  // create flags
        Shit::ImageType::TYPE_2D,
        _swapchain->GetCreateInfoPtr()->format,
        {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
        1,  // mipmap levels,
        1,  // array layers
        _sampleCount,
        Shit::ImageTiling::OPTIMAL,
        Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::TRANSFER_SRC_BIT,
        Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

    Shit::ImageViewCreateInfo colorImageViewCI{0,
                                               Shit::ImageViewType::TYPE_2D,
                                               _swapchain->GetCreateInfoPtr()->format,
                                               {},
                                               {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

    //================
    Shit::ImageCreateInfo depthImageCI{
        {},  // create flags
        Shit::ImageType::TYPE_2D,
        _defaultRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[1].format,
        {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
        1,  // mipmap levels,
        1,  // array layers
        _sampleCount,
        Shit::ImageTiling::OPTIMAL,
        Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
        Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

    Shit::ImageViewCreateInfo depthImageViewCI{
        0,
        Shit::ImageViewType::TYPE_2D,
        _defaultRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[1].format,
        {},
        {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}};

    Shit::FramebufferCreateInfo frameBufferCI{_defaultRenderTarget.renderPass,
                                              _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount, 0,
                                              Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                                                             _swapchain->GetCreateInfoPtr()->imageExtent.height},
                                              1};

    std::vector<Shit::ImageView *> attachments;
    auto imageCount = _swapchain->GetImageCount();
    for (int i = 0; i < imageCount; ++i) {
        if (_sampleCount == Shit::SampleCountFlagBits::BIT_1) {
            colorImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            depthImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));
        } else {
            // color image
            colorImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(colorImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(colorImageViewCI));

            //
            depthImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            // resolve image
            colorImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));
        }
        frameBufferCI.pAttachments = &_defaultRenderTarget.imageViews[i * frameBufferCI.attachmentCount];
        _defaultRenderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
    }
}

void AppBase::createCommandPool() {
    _shortLiveCommandPool = _device->Create(Shit::CommandPoolCreateInfo{
        Shit::CommandPoolCreateFlagBits::TRANSIENT_BIT | Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
        _graphicsQueue->GetFamilyIndex()});
    _longLiveCommandPool = _device->Create(Shit::CommandPoolCreateInfo{
        Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT, _graphicsQueue->GetFamilyIndex()});
}
void AppBase::createSyncObjects() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        _imageAvailableSemaphores[i] = _device->Create(Shit::SemaphoreCreateInfo{});
        _renderFinishedSemaphores[i] = _device->Create(Shit::SemaphoreCreateInfo{});
        _inFlightFences[i] = _device->Create(Shit::FenceCreateInfo{Shit::FenceCreateFlagBits::SIGNALED_BIT});
    }
}
void AppBase::createDescriptorPool() {
    Shit::DescriptorPoolSize poolsize[]{
        {Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 500}, {Shit::DescriptorType::STORAGE_IMAGE, 500},
        {Shit::DescriptorType::UNIFORM_BUFFER, 500},         {Shit::DescriptorType::UNIFORM_BUFFER_DYNAMIC, 500},
        {Shit::DescriptorType::STORAGE_BUFFER, 500},         {Shit::DescriptorType::STORAGE_BUFFER_DYNAMIC, 500},
    };
    _descriptorPool = _device->Create(Shit::DescriptorPoolCreateInfo{1000, std::size(poolsize), poolsize});
}
void AppBase::initDefaultCommandBuffer() {
    _longLiveCommandPool->CreateCommandBuffers(
        Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, MAX_FRAMES_IN_FLIGHT}, _defaultCommandBuffers);

    // Shit::ImageSubresourceRange ranges[]{
    //	{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
}
void AppBase::recordDefaultCommandBuffer(uint32_t imageIndex) {
    auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
    // cmdBuffer->Reset();
    cmdBuffer->Begin(Shit::CommandBufferBeginInfo{Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
    //_defaultCommandBuffers[i]->ClearColorImage(Shit::ClearColorImageInfo{
    //	_swapchain->GetImageByIndex(i),
    //	Shit::ImageLayout::TRANSFER_DST_OPTIMAL,
    //	clearValue,
    //	1,
    //	ranges});
    cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                         _defaultRenderTarget.frameBuffers[imageIndex],
                                                         {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                         (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                         _defaultRenderTarget.clearValues.data(),
                                                         Shit::SubpassContents::INLINE});
    cmdBuffer->EndRenderPass();
    cmdBuffer->End();
}
void AppBase::createQueues() {
    // present queue
    auto presentQueueFamily = _surface->GetPresentQueueFamily();
    _presentQueue = _device->GetQueue(presentQueueFamily->index, 0);

    // graphics queue
    // 5
    auto graphicsQueueFamilyProperty = _renderSystem->GetQueueFamily(Shit::QueueFlagBits::GRAPHICS_BIT);
    if (!graphicsQueueFamilyProperty.has_value()) THROW("failed to find a graphic queue");
    _graphicsQueue = _device->GetQueue(graphicsQueueFamilyProperty->index, 0);

    // transfer queue
    auto transferQueueFamilyProperty = _renderSystem->GetQueueFamily(Shit::QueueFlagBits::TRANSFER_BIT);
    if (!transferQueueFamilyProperty.has_value()) THROW("failed to find a transfer queue");
    _transferQueue = _device->GetQueue(transferQueueFamilyProperty->index, 0);

    // compute queue
    auto computeQueueFamilyProperty = _renderSystem->GetQueueFamily(Shit::QueueFlagBits::COMPUTE_BIT);
    if (!computeQueueFamilyProperty.has_value()) THROW("failed to find a compute queue");
    _computeQueue = _device->GetQueue(computeQueueFamilyProperty->index, 0);
}
void AppBase::updateScene() {
    for (auto e : _behaviours) {
        e->updatePerFrame(_frameDeltaTimeMs);
    }

    // updae scene
    for (auto &&e : _gameObjects) {
        e->update();
    }
}
void AppBase::mainLoop() {
    _timer.Restart();
    while (!glfwWindowShouldClose(_window)) {
        glfwPollEvents();
        //_device->WaitIdle();
        _frameDeltaTimeMs = _timer.GetDt();

        _inFlightFences[_currentFrame]->WaitFor(UINT64_MAX);

        uint32_t imageIndex{};
        Shit::GetNextImageInfo nextImageInfo{UINT64_MAX, _imageAvailableSemaphores[_currentFrame]};
        auto ret = _swapchain->GetNextImage(nextImageInfo, imageIndex);
        if (ret == Shit::Result::SHIT_ERROR_OUT_OF_DATE || _needRecreateSwapchain) {
            _device->WaitIdle();
            recreateSwapchain();
            continue;
        } else if (ret != Shit::Result::SUCCESS && ret != Shit::Result::SUBOPTIMAL) {
            THROW("failed to get next image");
        }
        _inFlightFences[_currentFrame]->Reset();

        updateScene();
        updateFrame(imageIndex);
        _editorCamera->updateUBOBuffer(imageIndex);

        recordDefaultCommandBuffer(imageIndex);
        renderImGui(imageIndex);

        setupSubmitCommandBuffers(_currentFrame);

        std::vector<Shit::SubmitInfo> submitInfos{{1, &_imageAvailableSemaphores[_currentFrame],
                                                   (uint32_t)_submitCommandBuffers.size(), _submitCommandBuffers.data(),
                                                   1, &_renderFinishedSemaphores[_currentFrame]}};
        _graphicsQueue->Submit(submitInfos, _inFlightFences[_currentFrame]);

        Shit::PresentInfo presentInfo{1, &_renderFinishedSemaphores[_currentFrame], 1, &_swapchain, &imageIndex};
        auto res = _presentQueue->Present(presentInfo);
        if (res == Shit::Result::SHIT_ERROR_OUT_OF_DATE || res == Shit::Result::SUBOPTIMAL) {
            _needRecreateSwapchain = true;
            _device->WaitIdle();
            recreateSwapchain();
        } else if (res != Shit::Result::SUCCESS) {
            THROW("failed to present swapchain image");
        }
        if (_takeScreenShot) {
            takeScreenshot(_device, _swapchain->GetImageByIndex(imageIndex), Shit::ImageLayout::PRESENT_SRC);
            _takeScreenShot = false;
        }
        _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    _device->WaitIdle();
}
void AppBase::setupSubmitCommandBuffers(uint32_t frameIndex) {
    _submitCommandBuffers = {_defaultCommandBuffers[frameIndex], _imguiCommandBuffers[frameIndex]};
}
void AppBase::createEditorCamera() {
    // create camera
    auto cameraNode = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
    _editorCamera = cameraNode->addComponent<Camera>(
        PerspectiveDescription{glm::radians(60.f), float(_framebufferWidth) / _framebufferHeight}, 0.01, 500.f);
    _editorCamera->getParentTransform()->translate({0, 0, 3});
    cameraNode->addComponent<EditCameraController>(_window);
}
Shit::Sampler *AppBase::createSampler(std::string_view name, Shit::SamplerCreateInfo const &createInfo) {
    return _samplers.emplace(std::string(name), _device->Create(createInfo)).first->second;
}
void AppBase::createSamplers() {
    createSampler("linear", Shit::SamplerCreateInfo{Shit::Filter::LINEAR,
                                                    Shit::Filter::LINEAR,
                                                    Shit::SamplerMipmapMode::NEAREST,
                                                    Shit::SamplerWrapMode::REPEAT,
                                                    Shit::SamplerWrapMode::REPEAT,
                                                    Shit::SamplerWrapMode::REPEAT,
                                                    0,
                                                    false,
                                                    0,
                                                    false,
                                                    {},
                                                    -1000,
                                                    1000,
                                                    Shit::BorderColor::FLOAT_OPAQUE_BLACK});
    createSampler("nearest", Shit::SamplerCreateInfo{Shit::Filter::NEAREST,
                                                     Shit::Filter::NEAREST,
                                                     Shit::SamplerMipmapMode::NEAREST,
                                                     Shit::SamplerWrapMode::REPEAT,
                                                     Shit::SamplerWrapMode::REPEAT,
                                                     Shit::SamplerWrapMode::REPEAT,
                                                     0,
                                                     false,
                                                     0,
                                                     false,
                                                     {},
                                                     -1000,
                                                     1000,
                                                     Shit::BorderColor::FLOAT_OPAQUE_BLACK});
    createSampler("trilinear", Shit::SamplerCreateInfo{Shit::Filter::LINEAR,
                                                       Shit::Filter::LINEAR,
                                                       Shit::SamplerMipmapMode::LINEAR,
                                                       Shit::SamplerWrapMode::REPEAT,
                                                       Shit::SamplerWrapMode::REPEAT,
                                                       Shit::SamplerWrapMode::REPEAT,
                                                       0,
                                                       false,
                                                       0,
                                                       false,
                                                       {},
                                                       -1000,
                                                       1000,
                                                       Shit::BorderColor::FLOAT_OPAQUE_BLACK});
}
void AppBase::createDescriptorSetLayouts() {
    //
    Shit::DescriptorSetLayoutBinding bindings[]{
        // transform
        {0, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::VERTEX_BIT},
        // camera
        {0, Shit::DescriptorType::STORAGE_BUFFER, 1,
         Shit::ShaderStageFlagBits::VERTEX_BIT | Shit::ShaderStageFlagBits::FRAGMENT_BIT |
             Shit::ShaderStageFlagBits::GEOMETRY_BIT | Shit::ShaderStageFlagBits::COMPUTE_BIT},
        // material
        {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 8, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        {1, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        // light
        {1, Shit::DescriptorType::STORAGE_BUFFER, 1,
         Shit::ShaderStageFlagBits::FRAGMENT_BIT | Shit::ShaderStageFlagBits::GEOMETRY_BIT},
        // env
        //{3, Shit::DescriptorType::UNIFORM_BUFFER, 1,
        // Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        //{8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
        // Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        //{9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
        // Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        //{10, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
        // Shit::ShaderStageFlagBits::FRAGMENT_BIT},
    };

    _transformDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[0]});
    _cameraDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[1]});
    _materialDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{2, &bindings[2]});
    _lightDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[4]});
    //_envDescriptorSetLayout =
    //_device->Create(Shit::DescriptorSetLayoutCreateInfo{4, &bindings[5]});
}
void AppBase::initImGui() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
    // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
    // Enable Gamepad Controls
    int frameWidth, frameHeight;
    glfwGetFramebufferSize(_window, &frameWidth, &frameHeight);
    io.DisplaySize = ImVec2{float(frameWidth), float(frameHeight)};

    _imguiCommandBuffers.resize(getFramesInFlight());
    _longLiveCommandPool->CreateCommandBuffers(
        Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, getFramesInFlight()},
        _imguiCommandBuffers.data());

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    auto imageCount = _swapchain->GetImageCount();
    std::vector<Shit::Image *> images(imageCount);
    _swapchain->GetImages(images.data());

    ImGui_ImplShitRenderer_InitInfo imguiInitInfo{MAX_FRAMES_IN_FLIGHT,
                                                  _rendererVersion,
                                                  _device,
                                                  images,
                                                  _imguiCommandBuffers,
                                                  Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                  Shit::ImageLayout::PRESENT_SRC};

    ImGui_ImplShitRenderer_Init(&imguiInitInfo);
    _device->ExecuteOneTimeCommand(
        [](Shit::CommandBuffer *commandBuffer) { ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer); });
    ImGui_ImplShitRenderer_DestroyFontUploadObjects();
    addRecreateSwapchainListener(std::bind(&AppBase::ImguiRecreateSwapchain, this));
}
void AppBase::ImguiRecreateSwapchain() {
    std::vector<Shit::Image *> images(_swapchain->GetImageCount());
    _swapchain->GetImages(images.data());
    ImGui_ImplShitRenderer_RecreateFrameBuffers(images);
}
void AppBase::renderImGui(uint32_t imageIndex) {
    ImGui_ImplShitRenderer_NewFrame();
    ImGui::NewFrame();
    //=======================================
    // ImGui::Begin("info");
    // ImGui::Text("FPS:%0.f", 1000.f / _frameDeltaTimeMs);
    // ImGui::End();
    setupImGui();
    //=======================================
    ImGui::Render();
    ImGui_ImplShitRenderer_RecordCommandBuffer(_currentFrame, imageIndex);
}
void AppBase::setupImGui() {
    static bool isOpen = true;
    ImGui::ShowDemoWindow(&isOpen);
}
void AppBase::run(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle,
                  Shit::SampleCountFlagBits sampleCount) {
    init(rendererVersion, windowTitle, sampleCount);
    initImGui();

    createEditorCamera();
    createSamplers();

    prepare();

    for (auto &&e : _gameObjects) {
        e->getComponents(_behaviours);
        e->prepare();
    }

    updateScene();

    mainLoop();

    cleanUp();
}
void AppBase::onFramebufferResize(int width, int height) {
    ImGuiIO &io = ImGui::GetIO();
    _framebufferWidth = io.DisplaySize.x = width;
    _framebufferHeight = io.DisplaySize.y = height;
    while (_framebufferWidth == 0 || _framebufferHeight == 0) {
        glfwWaitEvents();
        glfwGetFramebufferSize(_window, &_framebufferWidth, &_framebufferHeight);
    }
    onFramebufferResizeImpl(width, height);
}
void AppBase::onMouseButton(int button, int action, int mods) {
    ImGuiIO &io = ImGui::GetIO();
    io.MouseDown[0] = (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
    io.MouseDown[1] = (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS);
    io.MouseDown[2] = (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS);

    if (!io.WantCaptureMouse) onMouseButtonImpl(button, action, mods);
}
void AppBase::onMouseMove(double xpos, double ypos) {
    ImGuiIO &io = ImGui::GetIO();
    io.MousePos.x = xpos;
    io.MousePos.y = ypos;
    if (!io.WantCaptureMouse) onMouseMoveImpl(xpos, ypos);
}
void AppBase::onScroll(double xoffset, double yoffset) {
    ImGuiIO &io = ImGui::GetIO();
    io.MouseWheelH = xoffset;
    io.MouseWheel = yoffset;
    if (!io.WantCaptureMouse) onScrollImpl(xoffset, yoffset);
}
void AppBase::onKeyboard(int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) glfwSetWindowShouldClose(_window, GL_TRUE);
    if (key == GLFW_KEY_P && action == GLFW_RELEASE) _takeScreenShot = true;

    ImGuiIO &io = ImGui::GetIO();

    if (!io.WantCaptureKeyboard) onKeyboardImpl(key, scancode, action, mods);
}