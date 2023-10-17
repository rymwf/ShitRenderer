#include "common/appbase.h"
#include "common/entry.h" #include < stb_image.h>

#define MAX_FRAMES_IN_FLIGHT 2
#define SAMPLE_COUNT Shit::SampleCountFlagBits::BIT_1

#define INPUT_IMAGE_COUNT 4

uint32_t WIDTH = 800, HEIGHT = 600;

static const char *vertShaderName = "raytracing/render_in_triangle.vert";

static const char *fragShaderNames[]{
    "raytracing/raytracing_sphere.frag", "raytracing/raymarching_sphere.frag",
    "raytracing/BSpline.frag",           "raytracing/glass.frag",
    "raytracing/boundingVolume.frag",
};
static const char *inputImageNames[INPUT_IMAGE_COUNT]{
    "00000010.jpg",
    "Ridgecrest_Road_Ref.hdr",
    "envBRDF.hdr",
};

std::vector<uint16_t> indices{0, 1, 2};

struct UBO {
    glm::vec3 iResolution;
    float iTime;
    float iChannelTime[4];
    glm::vec4 iMouse;
    glm::vec4 iDate;
    glm::vec4 iChannelResolution[4];
    float iTimeDelta;
    float iFrame;
    float iSampleRate;
};

static int curPipelineIndex = 0;

class Hello {

    GLFWwindow *window;
    Shit::Surface *surface;

    Shit::RenderSystem *renderSystem;
    Shit::Device *device;

    Shit::SwapchainCreateInfo swapchainCreateInfo;
    Shit::Swapchain *swapchain;
    std::vector<Shit::ImageView *> swapchainImageViews;
    std::vector<Shit::Framebuffer *> framebuffers;

    Shit::CommandPool *commandPool;
    std::vector<Shit::CommandBuffer *> commandBuffers;
    Shit::Queue *graphicsQueue;
    Shit::Queue *presentQueue;
    Shit::Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];

    Shit::Shader *vertShader;
    std::vector<Shit::Shader *> fragShaders;

    Shit::PipelineLayout *pipelineLayout;
    Shit::RenderPass *renderPass;
    std::vector<Shit::Pipeline *> pipelines;

    Shit::Buffer *drawIndirectCmdBuffer;
    Shit::Buffer *drawCountBuffer;
    Shit::Buffer *drawIndexedIndirectCmdBuffer;
    Shit::Buffer *indexBuffer;

    Shit::DescriptorPool *descriptorPool;
    Shit::DescriptorSetLayout *descriptorSetLayout;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    std::vector<Shit::Buffer *> uboBuffers;

    // bool uboFlags[MAX_FRAMES_IN_FLIGHT]{};
    UBO ubo{};

    Shit::Sampler *linearSampler;
    Shit::Image *inputImages[INPUT_IMAGE_COUNT];
    Shit::ImageView *inputImageViews[INPUT_IMAGE_COUNT];

    float curTime;  // seconds
    uint32_t currentFrame{};

    ///=========================
    // GUI
    Shit::CommandPool *guiCommandPool;
    std::vector<Shit::CommandBuffer *> guiCommandBuffers;  // secondary commandbuffers
    Shit::RenderPass *guiRenderPass;

public:
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, nullptr, nullptr);

        glfwSetKeyCallback(window, &Hello::key_callback);
        glfwSetFramebufferSizeCallback(window, &Hello::framebuffer_size_callback);
        glfwSetCursorPosCallback(window, &Hello::cursor_position_callback);
        glfwSetMouseButtonCallback(window, &Hello::mouse_button_callback);
        glfwSetScrollCallback(window, &Hello::scroll_callback);

        glfwSetWindowUserPointer(window, this);
    }
    void initRenderSystem() {
        Shit::RenderSystemCreateInfo renderSystemCreateInfo{g_RendererVersion,
                                                            Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT};

        renderSystem = LoadRenderSystem(renderSystemCreateInfo);
        g_RendererVersion = renderSystem->GetCreateInfo()->version;

        // create surface
#ifdef _WIN32
        surface = renderSystem->CreateSurface(Shit::SurfaceCreateInfoWin32{glfwGetWin32Window(window)});
#endif
        // create logical device
        device = renderSystem->CreateDevice({});

        // 3
        auto graphicsQueueFamilyProperty = renderSystem->GetQueueFamily(Shit::QueueFlagBits::GRAPHICS_BIT);
        if (!graphicsQueueFamilyProperty.has_value()) THROW("failed to find a graphic queue")

        LOG_VAR(graphicsQueueFamilyProperty->index);
        LOG_VAR(graphicsQueueFamilyProperty->count);

        // 4. command pool
        Shit::CommandPoolCreateInfo commandPoolCreateInfo{Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT,
                                                          graphicsQueueFamilyProperty->index};
        commandPool = device->Create(commandPoolCreateInfo);

        // 5
        graphicsQueue = device->GetQueue(graphicsQueueFamilyProperty->index, 0);

        createShaders();
        createDrawCommandBuffers();
        createSyncObjects();
        createIndexBuffer();

        createSwapchain();
        createRenderPasses();
        createFramebuffers();

        createDescriptorSets();
        createInputImages();
        createUBOBuffer();

        createPipelines();

        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        commandPool->CreateCommandBuffers(
            Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, swapchain->GetImageCount()},
            commandBuffers.data());

        ubo.iResolution.x = swapchain->GetCreateInfoPtr()->imageExtent.width;
        ubo.iResolution.y = swapchain->GetCreateInfoPtr()->imageExtent.height;

        // prepare GUI
        initGUI();
    }
    static void key_callback(GLFWwindow *pWindow, int key, int scancode, int action, int mods) {
        ImGuiIO &io = ImGui::GetIO();
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
        io.KeysDown[ImGuiKey_W] = key == GLFW_KEY_W && action == GLFW_PRESS;
        io.KeysDown[ImGuiKey_S] = key == GLFW_KEY_S && action == GLFW_PRESS;
        io.KeysDown[ImGuiKey_A] = key == GLFW_KEY_A && action == GLFW_PRESS;
        io.KeysDown[ImGuiKey_D] = key == GLFW_KEY_D && action == GLFW_PRESS;
    }
    static void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
        auto app = (Hello *)glfwGetWindowUserPointer(window);

        ImGuiIO &io = ImGui::GetIO();
        io.MouseDown[0] = button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS;
        io.MouseDown[1] = button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS;
        io.MouseDown[2] = button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS;
        if (!io.WantCaptureMouse) {
            if (io.MouseDown[0] || io.MouseDown[1]) {
                app->ubo.iMouse[0] = io.MousePos.x / io.DisplaySize.x;
                app->ubo.iMouse[1] = 1. - io.MousePos.y / io.DisplaySize.y;
                app->ubo.iMouse[2] = 1;
            } else {
                app->ubo.iMouse = {};
            }
        }
    }
    static void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
        auto app = (Hello *)glfwGetWindowUserPointer(window);
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos.x = xpos;
        io.MousePos.y = ypos;
        if (!io.WantCaptureMouse) {
            //    for (auto &&e : uboFlags)
            // 	   e = true;
            if (io.MouseDown[0] || io.MouseDown[1]) {
                app->ubo.iMouse[0] = float(xpos) / io.DisplaySize.x;
                app->ubo.iMouse[1] = 1. - float(ypos) / io.DisplaySize.y;
            } else {
                app->ubo.iMouse = {};
            }
        }
    }
    static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) { 
        auto app = (Hello *)glfwGetWindowUserPointer(window);
        ImGuiIO &io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            io.MouseWheel += yoffset;
            io.MouseWheelH += xoffset;
        }
    }
    static void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
        auto app = (Hello *)glfwGetWindowUserPointer(window);
        ImGuiIO &io = ImGui::GetIO();
        app->ubo.iResolution.x = io.DisplaySize.x = width;
        app->ubo.iResolution.y = io.DisplaySize.y = height;
    }
    void mainLoop() {
        auto startTime = std::chrono::high_resolution_clock::now();
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            curTime = float(std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::high_resolution_clock::now() - startTime)
                                .count()) /
                      1000;
            ++ubo.iFrame;
            drawFrame();
        }
        presentQueue->WaitIdle();
    }
    void cleanUp() {}
    void run() {
        initWindow();
        initRenderSystem();
        mainLoop();
        cleanUp();
    }
    void cleanupSwapchain() {}
    void recreateSwapchain() {
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        while (framebufferWidth == 0 || framebufferHeight == 0) {
            glfwWaitEvents();
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        }

        for (auto &&e : framebuffers) device->Destroy(e);
        // resize swapchain

        swapchain->Resize(framebufferWidth, framebufferHeight);

        createFramebuffers();
        guiRecreateSwapchain();
    }
    void drawFrame() {
        inFlightFences[currentFrame]->WaitFor(UINT64_MAX);

        uint32_t imageIndex{};
        Shit::GetNextImageInfo nextImageInfo{UINT64_MAX, imageAvailableSemaphores[currentFrame]};
        auto res = swapchain->GetNextImage(nextImageInfo, imageIndex);
        if (res == Shit::Result::SHIT_ERROR_OUT_OF_DATE) {
            presentQueue->WaitIdle();
            recreateSwapchain();
            return;
        } else if (res != Shit::Result::SUCCESS && res != Shit::Result::SUBOPTIMAL) {
            THROW("failed to get next image");
        }
        //==================================
        inFlightFences[currentFrame]->Reset();

        updateUBOBuffer(imageIndex);

        drawGUI(currentFrame, imageIndex);
        updateCommandBuffer(commandBuffers[currentFrame], pipelines[curPipelineIndex], imageIndex);
        //=========================
        Shit::CommandBuffer *cmdBuffers[] = {commandBuffers[currentFrame], guiCommandBuffers[currentFrame]};
        std::vector<Shit::SubmitInfo> submitInfos{
            {1, &imageAvailableSemaphores[currentFrame], 2, cmdBuffers, 1, &renderFinishedSemaphores[currentFrame]}};
        graphicsQueue->Submit(submitInfos, inFlightFences[currentFrame]);

        Shit::PresentInfo presentInfo{1, &renderFinishedSemaphores[currentFrame], 1, &swapchain, &imageIndex};
        res = presentQueue->Present(presentInfo);
        if (res == Shit::Result::SHIT_ERROR_OUT_OF_DATE || res == Shit::Result::SUBOPTIMAL) {
            presentQueue->WaitIdle();
            recreateSwapchain();
        } else if (res != Shit::Result::SUCCESS) {
            THROW("failed to present swapchain image");
        }
        // presentQueue->WaitIdle();
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void createShaders() {
        auto vertShaderPath = buildShaderPath(device, vertShaderName, g_RendererVersion);
        auto vertCode = readFile(vertShaderPath.c_str());
        vertShader = device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, vertCode.size(), vertCode.data()});

        for (auto e : fragShaderNames) {
            auto fragShaderPath = buildShaderPath(device, e, g_RendererVersion);
            auto fragCode = readFile(fragShaderPath.c_str());
            fragShaders.emplace_back(
                device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, fragCode.size(), fragCode.data()}));
        }
    }

    void createSwapchain() {
        auto swapchainFormat = chooseSwapchainFormat(
            {
                {Shit::Format::B8G8R8A8_SRGB, Shit::ColorSpace::SRGB_NONLINEAR},
                {Shit::Format::R8G8B8A8_SRGB, Shit::ColorSpace::SRGB_NONLINEAR},
            },
            surface);
        LOG_VAR(static_cast<int>(swapchainFormat.format));
        LOG_VAR(static_cast<int>(swapchainFormat.colorSpace));

        auto presentMode = choosePresentMode({Shit::PresentMode::IMMEDIATE, Shit::PresentMode::FIFO}, surface);

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        // set imgui
        swapchain = surface->Create(Shit::SwapchainCreateInfo{device,
                                                              2,  // min image count
                                                              swapchainFormat.format,
                                                              swapchainFormat.colorSpace,
                                                              {(uint32_t)framebufferWidth, (uint32_t)framebufferHeight},
                                                              1,
                                                              Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                                              presentMode});

        auto presentQueueFamily = surface->GetPresentQueueFamily();
        presentQueue = device->GetQueue(presentQueueFamily->index, 0);
    }
    void createRenderPasses() {
        std::vector<Shit::AttachmentDescription> attachmentDescriptions{
            {swapchain->GetCreateInfoPtr()->format, SAMPLE_COUNT, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED,  // inital layout
             Shit::ImageLayout::PRESENT_SRC}};

        std::vector<Shit::AttachmentReference> colorAttachments{
            {0,  // the index of attachment description
             Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };

        std::vector<Shit::SubpassDescription> subPasses{
            Shit::SubpassDescription{
                Shit::PipelineBindPoint::GRAPHICS,
                0,
                0,
                (uint32_t)colorAttachments.size(),
                colorAttachments.data(),
            },
        };
        Shit::RenderPassCreateInfo renderPassCreateInfo{
            (uint32_t)attachmentDescriptions.size(),
            attachmentDescriptions.data(),
            (uint32_t)subPasses.size(),
            subPasses.data(),
        };

        renderPass = device->Create(renderPassCreateInfo);
    }
    void createPipelines() {
        Shit::VertexInputStateCreateInfo vertexInputState{};
        Shit::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
            Shit::PrimitiveTopology::TRIANGLE_LIST,
        };
        // Shit::PipelineViewportStateCreateInfo viewportStateCreateInfo{
        //	{{0, 0, 800, 600, 0, 1}},
        //	{{{0, 0}, {500, 400}}}};
        Shit::Viewport viewports[]{{0, 0, static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width),
                                    static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height), 0, 1}};
        Shit::Rect2D scissors[]{{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent}};
        Shit::PipelineViewportStateCreateInfo viewportStateCreateInfo{(uint32_t)std::size(viewports), viewports,
                                                                      (uint32_t)std::size(scissors), scissors};
        Shit::PipelineTessellationStateCreateInfo tessellationState{};
        Shit::PipelineRasterizationStateCreateInfo rasterizationState{false,
                                                                      false,
                                                                      Shit::PolygonMode::FILL,
                                                                      Shit::CullMode::BACK,
                                                                      Shit::FrontFace::COUNTER_CLOCKWISE,
                                                                      false,
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      1.f};

        Shit::PipelineMultisampleStateCreateInfo multisampleState{
            SAMPLE_COUNT,
        };
        Shit::PipelineDepthStencilStateCreateInfo depthStencilState{};

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentstate{
            false,
            Shit::BlendFactor::SRC_ALPHA,
            Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
            Shit::BlendOp::ADD,
            {},
            {},
            {},
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
                Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT};
        Shit::PipelineColorBlendStateCreateInfo colorBlendState{
            false,
            Shit::LogicOp::COPY,
            1,
            &colorBlendAttachmentstate,
        };
        Shit::DynamicState dynamicStates[]{
            Shit::DynamicState::VIEWPORT,
            Shit::DynamicState::SCISSOR,
        };
        Shit::PipelineDynamicStateCreateInfo dynamicStateCI{std::size(dynamicStates), dynamicStates};
        for (auto e : fragShaders) {
            std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCreateInfos{
                Shit::PipelineShaderStageCreateInfo{
                    Shit::ShaderStageFlagBits::VERTEX_BIT,
                    vertShader,
                    "main",
                },
                Shit::PipelineShaderStageCreateInfo{
                    Shit::ShaderStageFlagBits::FRAGMENT_BIT,
                    e,
                    "main",
                },
            };
            pipelines.emplace_back(
                device->Create(Shit::GraphicsPipelineCreateInfo{(uint32_t)shaderStageCreateInfos.size(),
                                                                shaderStageCreateInfos.data(),
                                                                vertexInputState,
                                                                inputAssemblyState,
                                                                viewportStateCreateInfo,
                                                                tessellationState,
                                                                rasterizationState,
                                                                multisampleState,
                                                                depthStencilState,
                                                                colorBlendState,
                                                                dynamicStateCI,
                                                                pipelineLayout,
                                                                renderPass,
                                                                0}));
        }
    }
    void createFramebuffers() {
        Shit::FramebufferCreateInfo framebufferCreateInfo{renderPass, 0, nullptr,
                                                          swapchain->GetCreateInfoPtr()->imageExtent, 1};
        auto count = swapchain->GetImageCount();
        framebuffers.resize(count);
        framebufferCreateInfo.attachmentCount = 1;
        for (uint32_t i = 0; i < count; ++i) {
            auto imageView = swapchain->GetImageViewByIndex(i);
            framebufferCreateInfo.pAttachments = &imageView;
            framebuffers[i] = device->Create(framebufferCreateInfo);
        }
    }
    void updateCommandBuffer(Shit::CommandBuffer* cmdBuffer ,Shit::Pipeline *pipeline, uint32_t imageIndex) {
        static std::vector<Shit::ClearValue> clearValues{std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f}};

        device->WaitIdle();

        Shit::CommandBufferBeginInfo cmdBufferBeginInfo{};

        Shit::BeginRenderPassInfo renderPassBeginInfo{renderPass,
                                                      nullptr,
                                                      Shit::Rect2D{{}, swapchain->GetCreateInfoPtr()->imageExtent},
                                                      static_cast<uint32_t>(clearValues.size()),
                                                      clearValues.data(),
                                                      Shit::SubpassContents::INLINE};

        auto &&ext = swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{0, 0}, {ext.width, ext.height}};

        renderPassBeginInfo.pFramebuffer = framebuffers[imageIndex];
        cmdBuffer->Reset({});
        cmdBuffer->Begin(cmdBufferBeginInfo);
        cmdBuffer->BeginRenderPass(renderPassBeginInfo);
        cmdBuffer->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, pipeline});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0,
                                                                   1, &descriptorSets[imageIndex]});
        int drawMethod = 1;
        switch (drawMethod) {
            case 0:
            default: {
                cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
            } break;
            case 1: {
                cmdBuffer->DrawIndirect(
                    Shit::DrawIndirectInfo{drawIndirectCmdBuffer, 0, 1, sizeof(Shit::DrawIndirectCommand)});
            } break;
        }
        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }

    void createSyncObjects() {
        for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            imageAvailableSemaphores[i] = device->Create(Shit::SemaphoreCreateInfo{});
            renderFinishedSemaphores[i] = device->Create(Shit::SemaphoreCreateInfo{});
            inFlightFences[i] = device->Create(Shit::FenceCreateInfo{Shit::FenceCreateFlagBits::SIGNALED_BIT});
        }
    }
    void createDrawCommandBuffers() {
        // create draw indirect command buffer
        std::vector<Shit::DrawIndirectCommand> drawIndirectCmds{{3, 1, 0, 0}};
        Shit::BufferCreateInfo bufferCreateInfo{
            {},
            sizeof(Shit::DrawIndirectCommand) * drawIndirectCmds.size(),
            Shit::BufferUsageFlagBits::INDIRECT_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        drawIndirectCmdBuffer = device->Create(bufferCreateInfo, drawIndirectCmds.data());
        bufferCreateInfo.size = sizeof(uint32_t);
        uint32_t drawCount = 1;
        drawCountBuffer = device->Create(bufferCreateInfo, &drawCount);

        std::vector<Shit::DrawIndexedIndirectCommand> drawIndexedIndirectCmds{
            {static_cast<uint32_t>(indices.size()), 1, 0, 0, 0}};

        // create draw indexed indirect command buffer
        Shit::BufferCreateInfo drawIndexedIndirectCmdBufferCreateInfo{
            {},
            sizeof(Shit::DrawIndexedIndirectCommand) * drawIndexedIndirectCmds.size(),
            Shit::BufferUsageFlagBits::INDIRECT_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        drawIndexedIndirectCmdBuffer =
            device->Create(drawIndexedIndirectCmdBufferCreateInfo, drawIndexedIndirectCmds.data());
    }
    void createIndexBuffer() {
        Shit::BufferCreateInfo indexBufferCreateInfo{
            {},
            sizeof(indices[0]) * indices.size(),
            Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        indexBuffer = device->Create(indexBufferCreateInfo, indices.data());
    }
    void createDescriptorSets() {
        std::vector<Shit::DescriptorSetLayoutBinding> bindings{
            {0, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 4, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        descriptorSetLayout =
            device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)bindings.size(), bindings.data()});

        std::vector<Shit::DescriptorPoolSize> poolSizes{
            {Shit::DescriptorType::UNIFORM_BUFFER, swapchain->GetImageCount()},
        };
        descriptorPool = device->Create(
            Shit::DescriptorPoolCreateInfo{swapchain->GetImageCount(), (uint32_t)poolSizes.size(), poolSizes.data()});
        std::vector<Shit::DescriptorSetLayout *> setLayouts(swapchain->GetImageCount(), descriptorSetLayout);
        descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{(uint32_t)setLayouts.size(), setLayouts.data()},
                                 descriptorSets);
        pipelineLayout = device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});
    }
    void createInputImages() {
        linearSampler = device->Create(Shit::SamplerCreateInfo{
            Shit::Filter::LINEAR,
            Shit::Filter::LINEAR,
            Shit::SamplerMipmapMode::LINEAR,
            Shit::SamplerWrapMode::REPEAT,
            Shit::SamplerWrapMode::REPEAT,
            Shit::SamplerWrapMode::REPEAT,
            0,
            false,
            1.,
            false,
            {},
            -1000.f,
            1000.f,
        });
        //==============
        int width{1}, height{1}, components{4}, componentSize{};
        Shit::Format format = Shit::Format::R8G8B8A8_SRGB;
        for (int i = 0; i < INPUT_IMAGE_COUNT; ++i) {
            void *pixels{};
            if (inputImageNames[i]) {
                auto imagePath = std::string(SHIT_SOURCE_DIR) + "/assets/images/" + inputImageNames[i];
                pixels = loadImage(imagePath.data(), width, height, components, 4,
                                   componentSize);  // force load an alpha channel,even not exist
            }
            if (componentSize == 4) format = Shit::Format::R32G32B32A32_SFLOAT;
            inputImages[i] = device->Create(Shit::ImageCreateInfo{{},
                                                                  Shit::ImageType::TYPE_2D,
                                                                  format,
                                                                  {(uint32_t)width, (uint32_t)height, 1},
                                                                  0,
                                                                  1,
                                                                  Shit::SampleCountFlagBits::BIT_1,
                                                                  Shit::ImageTiling::OPTIMAL,
                                                                  Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
                                            Shit::ImageAspectFlagBits::COLOR_BIT, pixels);
            inputImages[i]->GenerateMipmap(Shit::Filter::LINEAR, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                           Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            inputImageViews[i] = device->Create(Shit::ImageViewCreateInfo{
                inputImages[i],
                Shit::ImageViewType::TYPE_2D,
                format,
                {},
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, inputImages[i]->GetCreateInfoPtr()->mipLevels, 0, 1}});
        }
    }
    void createUBOBuffer() {
        for (int i = 0; i < swapchain->GetImageCount(); ++i) {
            uboBuffers.emplace_back(device->Create(
                Shit::BufferCreateInfo{
                    {},
                    sizeof(UBO),
                    Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
                    Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT},
                &ubo));
        }

        std::vector<Shit::DescriptorImageInfo> imagesInfo;
        for (int j = 0; j < INPUT_IMAGE_COUNT; ++j)
            imagesInfo.emplace_back(linearSampler, inputImageViews[j], Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        std::vector<Shit::WriteDescriptorSet> writes;

        Shit::DescriptorBufferInfo bufferInfo{uboBuffers[0], 0ul, sizeof(UBO)};
        for (int i = 0; i < swapchain->GetImageCount(); ++i) {
            writes.emplace_back(Shit::WriteDescriptorSet{descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo});
            writes.emplace_back(descriptorSets[i], 1, 0, (uint32_t)imagesInfo.size(),
                                Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, imagesInfo.data());
        }
        device->UpdateDescriptorSets(writes, {});
    }
    void updateUBOBuffer(int index) {
        void *data;
        uboBuffers[index]->MapMemory(0, sizeof(UBO), &data);
        ubo.iTime = curTime;
        memcpy(data, &ubo, sizeof(UBO));
        uboBuffers[index]->UnMapMemory();
    }
    void initGUI() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable
        // Keyboard Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; //
        // Enable Gamepad Controls
        int windowWidth, windowHeight;
        glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
        io.DisplaySize = ImVec2{float(windowWidth), float(windowHeight)};

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        guiCommandPool = device->Create(Shit::CommandPoolCreateInfo{
            Shit::CommandPoolCreateFlagBits::RESET_COMMAND_BUFFER_BIT, graphicsQueue->GetFamilyIndex()});
        guiCommandBuffers.resize(swapchain->GetImageCount());
        guiCommandPool->CreateCommandBuffers(
            Shit::CommandBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, swapchain->GetImageCount()},
            guiCommandBuffers.data());

        std::vector<Shit::Image *> images(swapchain->GetImageCount());
        swapchain->GetImages(images.data());

        ImGui_ImplShitRenderer_InitInfo imguiInitInfo{
            MAX_FRAMES_IN_FLIGHT,
            g_RendererVersion,
            device,
            images,
            guiCommandBuffers,
            Shit::ImageLayout::PRESENT_SRC,
            Shit::ImageLayout::PRESENT_SRC,
        };
        ImGui_ImplShitRenderer_Init(&imguiInitInfo);

        device->ExecuteOneTimeCommand(
            [](Shit::CommandBuffer *commandBuffer) { ImGui_ImplShitRenderer_CreateFontsTexture(commandBuffer); });
        ImGui_ImplShitRenderer_DestroyFontUploadObjects();
    }
    void guiRecreateSwapchain() {
        std::vector<Shit::Image *> images(swapchain->GetImageCount());
        swapchain->GetImages(images.data());
        ImGui_ImplShitRenderer_RecreateFrameBuffers(images);
    }
    void drawGUI(uint32_t frameIndex, uint32_t imageIndex) {
        ImGui_ImplShitRenderer_NewFrame();
        ImGui::NewFrame();

        //=======================================
        ImGui::Begin("pipeline selector");
        if (ImGui::Button("screen shot"))
            takeScreenshot(device, swapchain->GetImageByIndex(imageIndex), Shit::ImageLayout::PRESENT_SRC);

        for (int i = 0, len = IM_ARRAYSIZE(fragShaderNames); i < len; ++i)
            ImGui::RadioButton(fragShaderNames[i], &curPipelineIndex, i);

        ImGui::Text("iTime: %f", ubo.iTime);
        ImGui::Text("mouse pos x: %f", ubo.iMouse[0]);
        ImGui::Text("mouse pos y: %f", ubo.iMouse[1]);
        ImGui::Text("viewportSize x: %f", ubo.iResolution[0]);
        ImGui::Text("viewportSize y: %f", ubo.iResolution[1]);

        ImGui::End();
        //=======================================
        ImGui::Render();
        ImGui_ImplShitRenderer_RecordCommandBuffer(frameIndex, imageIndex);
    }
};
EXAMPLE_MAIN(Hello)
