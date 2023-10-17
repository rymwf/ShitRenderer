#include <stb_image.h>

#include "common/entry.h"

uint32_t WIDTH = 800, HEIGHT = 600;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const char *testImagePath = SHIT_SOURCE_DIR "/assets/images/00000010.jpg";

std::string vertShaderPath;
std::string fragShaderPath;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    static Shit::VertexBindingDescription getVertexBindingDescription(uint32_t binding) {
        return {
            binding,
            sizeof(Vertex),
            0,
        };
    }
    static std::vector<Shit::VertexAttributeDescription> getVertexAttributeDescription(uint32_t startLocation,
                                                                                       uint32_t binding) {
        return {
            {startLocation + 0, binding, Shit::Format::R32G32B32_SFLOAT, offsetof(Vertex, pos)},
            {startLocation + 1, binding, Shit::Format::R32G32B32_SFLOAT, offsetof(Vertex, color)},
            {startLocation + 2, binding, Shit::Format::R32G32_SFLOAT, offsetof(Vertex, texCoord)},
        };
    }
    static uint32_t getLocationCount() { return 3; }
};

std::vector<Vertex> vertices{
    {{-1, -1, 0}, {1, 0, 0}, {0, 0}},
    {{1, -1, 0}, {0, 1, 0}, {1, 0}},
    {{-1, 1, 0}, {0, 0, 1}, {0, 1}},
    {{1, 1, 0}, {1, 1, 0}, {1, 1}},
};
std::vector<uint16_t> indices{0, 1, 2, 3};

struct alignas(16) UBO_MVP {
    glm::mat4 PVM;
};

class Hello {
    Shit::RenderSystem *renderSystem;
    GLFWwindow *window;
    Shit::Surface *surface;
    Shit::Device *device;

    Shit::Swapchain *swapchain;
    std::vector<Shit::Framebuffer *> framebuffers;

    Shit::CommandPool *commandPool;
    std::vector<Shit::CommandBuffer *> commandBuffers;
    Shit::Queue *graphicsQueue;
    Shit::Queue *presentQueue;

    Shit::Semaphore *imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Semaphore *renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Fence *inFlightFences[MAX_FRAMES_IN_FLIGHT];

    Shit::Shader *vertShader;
    Shit::Shader *fragShader;

    Shit::DescriptorPool *descriptorPool;
    std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts;
    std::vector<Shit::DescriptorSet *> descriptorSets;

    Shit::PipelineLayout *pipelineLayout;
    Shit::RenderPass *renderPass;
    Shit::Pipeline *pipeline;

    Shit::Buffer *drawIndirectCmdBuffer;
    Shit::Buffer *drawCountBuffer;
    Shit::Buffer *drawIndexedIndirectCmdBuffer;
    Shit::Buffer *indexBuffer;
    Shit::Buffer *vertexBuffer;

    Shit::Image *testImage;
    Shit::ImageView *testImageView;
    Shit::Sampler *sampler;

    uint32_t FPS;
    float frameTimeInterval_ms;
    std::chrono::system_clock::time_point curTime;
    std::chrono::system_clock::time_point startTime;

    uint32_t currentFrame{};

public:
    static void key_callback(GLFWwindow *pWindow, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
    }
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(800, 600, __FILE__, nullptr, nullptr);

        glfwSetKeyCallback(window, &Hello::key_callback);
    }
    void initRenderSystem() {
        startTime = std::chrono::system_clock::now();
        Shit::RenderSystemCreateInfo renderSystemCreateInfo{
            .version = g_RendererVersion,
            .flags = Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT,
        };

        renderSystem = LoadRenderSystem(renderSystemCreateInfo);

        device = renderSystem->CreateDevice();

        // create surface
#ifdef _WIN32
        surface = renderSystem->CreateSurface(Shit::SurfaceCreateInfoWin32{glfwGetWin32Window(window)});
#endif

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

        createSwapchain();

        createShaders();
        createDescriptorSets();
        createRenderPasses();

        Shit::PushConstantRange pushConstantRange{Shit::ShaderStageFlagBits::VERTEX_BIT, 0, 0, sizeof(float) * 16};
        pipelineLayout = device->Create(Shit::PipelineLayoutCreateInfo{
            (uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), 1, &pushConstantRange});
        createPipeline();
        createFramebuffers();
        createSyncObjects();

        createDrawCommandBuffers();
        createIndexBuffer();
        createVertexBuffer();
        createImages();
        createSamplers();

        updateDescriptorSets();

        createCommandBuffers();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            auto temptime = std::chrono::system_clock::now();
            frameTimeInterval_ms =
                std::chrono::duration<float, std::chrono::milliseconds::period>(temptime - curTime).count();
            curTime = temptime;
            FPS = static_cast<uint32_t>(1000 / frameTimeInterval_ms);
            glfwPollEvents();
            drawFrame();
        }
        presentQueue->WaitIdle();
    }
    void cleanUp() { glfwDestroyWindow(window); }
    void run() {
        initWindow();
        initRenderSystem();
        mainLoop();
        cleanUp();
    }
    void recreateSwapchain() {
        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        while (framebufferWidth == 0 || framebufferHeight == 0) {
            glfwWaitEvents();
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        }
        for (auto &&e : framebuffers) device->Destroy(e);

        swapchain->Resize(framebufferWidth, framebufferHeight);

        createFramebuffers();
    }
    void drawFrame() {
        inFlightFences[currentFrame]->WaitFor(UINT64_MAX);

        uint32_t imageIndex{};
        Shit::GetNextImageInfo nextImageInfo{UINT64_MAX, imageAvailableSemaphores[currentFrame]};
        auto ret = swapchain->GetNextImage(nextImageInfo, imageIndex);
        if (ret == Shit::Result::SHIT_ERROR_OUT_OF_DATE) {
            presentQueue->WaitIdle();
            recreateSwapchain();
            return;
        } else if (ret != Shit::Result::SUCCESS && ret != Shit::Result::SUBOPTIMAL) {
            THROW("failed to get next image");
        }
        //========================================
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
        //=================================
        inFlightFences[currentFrame]->Reset();

        std::vector<Shit::SubmitInfo> submitInfos{{1, &imageAvailableSemaphores[currentFrame], 1,
                                                   &commandBuffers[currentFrame], 1,
                                                   &renderFinishedSemaphores[currentFrame]}};
        graphicsQueue->Submit(submitInfos, inFlightFences[currentFrame]);

        Shit::PresentInfo presentInfo{1, &renderFinishedSemaphores[currentFrame], 1, &swapchain, &imageIndex};
        auto res = presentQueue->Present(presentInfo);
        if (res == Shit::Result::SHIT_ERROR_OUT_OF_DATE || res == Shit::Result::SUBOPTIMAL) {
            presentQueue->WaitIdle();
            recreateSwapchain();
        } else if (res != Shit::Result::SUCCESS) {
            THROW("failed to present swapchain image");
        }
        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void createShaders() {
        g_shaderSourceLanguage =
            chooseShaderShourceLanguage({Shit::ShadingLanguage::SPIRV, Shit::ShadingLanguage::GLSL}, device);

        std::string vertCode =
            "\
#version 460\n\
#ifdef VULKAN\n\
#define PUSH_CONSTANT push_constant\n\
#else\n\
#define PUSH_CONSTANT std430, binding=0\n\
#extension GL_EXT_scalar_block_layout: enable\n\
#endif\n\
layout(location = 0) in vec3 inPos;\n\
layout(location = 1) in vec3 inColor;\n\
layout(location = 2) in vec2 inTexCoord;\n\
struct VS_OUT { \n\
	vec3 color;\n\
	vec2 texCoord;\n\
};\n\
layout(location = 0) out VS_OUT vs_out;\n\
layout(PUSH_CONSTANT) uniform uPushConstant{\n\
	mat4 PVM;\n\
};\n\
void main() \n\
{\n\
  gl_Position = PVM*vec4(inPos, 1);\n\
  vs_out.color = inColor;\n\
  vs_out.texCoord= inTexCoord;\n\
}\n\
";
        std::string fragCode =
            "\
#version 460\n\
layout(location = 0) out vec4 outColor;\n\
struct VS_OUT { \n\
	vec3 color;\n\
	vec2 texCoord;\n\
};\n\
layout(location = 0) in VS_OUT fs_in;\n\
layout(binding=0) uniform sampler2D tex;\n\
void main() \n\
{\n\
  outColor = texture(tex,fs_in.texCoord);\n\
}\n\
";
        std::string vertCodeSpv =
            compileGlslToSpirv(vertCode, Shit::ShaderStageFlagBits::VERTEX_BIT, g_RendererVersion);
        std::string fragCodeSpv =
            compileGlslToSpirv(fragCode, Shit::ShaderStageFlagBits::FRAGMENT_BIT, g_RendererVersion);

        vertShader =
            device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, vertCodeSpv.size(), vertCodeSpv.data()});
        fragShader =
            device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, fragCodeSpv.size(), fragCodeSpv.data()});
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

        auto presentMode = choosePresentMode(
            {Shit::PresentMode::IMMEDIATE, Shit::PresentMode::MAILBOX, Shit::PresentMode::FIFO}, surface);

        int framebufferWidth, framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

        Shit::SurfaceCapabilities caps;
        surface->GetCapabilities(caps);
        auto imageCount = (std::min)(caps.minImageCount + 1, caps.maxImageCount);

        swapchain = surface->Create(Shit::SwapchainCreateInfo{device,
                                                              imageCount,  // min image count
                                                              swapchainFormat.format,
                                                              swapchainFormat.colorSpace,
                                                              {(uint32_t)framebufferWidth, (uint32_t)framebufferHeight},
                                                              1,
                                                              Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                                                              presentMode});

        auto presentQueueFamily = surface->GetPresentQueueFamily();
        presentQueue = device->GetQueue(presentQueueFamily->index, 0);
    }
    void createDescriptorSets() {
        std::vector<Shit::DescriptorSetLayoutBinding> bindings{
            Shit::DescriptorSetLayoutBinding{
                .binding = 0,
                .descriptorType = Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .stageFlags = Shit::ShaderStageFlagBits::FRAGMENT_BIT,
            },
        };
        auto count = swapchain->GetImageCount();
        descriptorSetLayouts.resize(
            count, device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)bindings.size(), bindings.data()}));

        std::vector<Shit::DescriptorPoolSize> poolSizes(bindings.size());
        std::transform(bindings.begin(), bindings.end(), poolSizes.begin(), [count](auto &&e) {
            return Shit::DescriptorPoolSize{e.descriptorType, e.descriptorCount * count};
        });
        Shit::DescriptorPoolCreateInfo descriptorPoolCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                                (uint32_t)poolSizes.size(), poolSizes.data()};
        descriptorPool = device->Create(descriptorPoolCreateInfo);

        Shit::DescriptorSetAllocateInfo allocInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};
        descriptorPool->Allocate(allocInfo, descriptorSets);
    }
    void updateDescriptorSets() {
        Shit::DescriptorImageInfo imagesInfo[]{
            {sampler, testImageView, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        std::vector<Shit::WriteDescriptorSet> writes{
            Shit::WriteDescriptorSet{
                nullptr,  // pDstset
                0,        // dst binding
                0,        // dstArrayElement
                1,
                Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                &imagesInfo[0],
            },
        };
        for (size_t i = 0, n = descriptorSets.size(); i < n; ++i) {
            for (auto &&a : writes) a.pDstSet = descriptorSets[i];
            device->UpdateDescriptorSets(writes, {});
        }
    }
    void createRenderPasses() {
        std::vector<Shit::AttachmentDescription> attachmentDescriptions{
            {swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
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
                nullptr,
                static_cast<uint32_t>(colorAttachments.size()),
                colorAttachments.data(),
            },
        };
        Shit::RenderPassCreateInfo renderPassCreateInfo{
            attachmentDescriptions.size(),
            attachmentDescriptions.data(),
            subPasses.size(),
            subPasses.data(),
        };

        renderPass = device->Create(renderPassCreateInfo);
    }
    void createPipeline() {
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCreateInfos{
            Shit::PipelineShaderStageCreateInfo{
                Shit::ShaderStageFlagBits::VERTEX_BIT,
                vertShader,
                "main",
            },
            Shit::PipelineShaderStageCreateInfo{
                Shit::ShaderStageFlagBits::FRAGMENT_BIT,
                fragShader,
                "main",
            },
        };
        auto vertexBindingDesc = Vertex::getVertexBindingDescription(0);
        auto vertexAttributeDesc = Vertex::getVertexAttributeDescription(0, 0);
        Shit::VertexInputStateCreateInfo vertexInputState{
            1,
            &vertexBindingDesc,
            (uint32_t)vertexAttributeDesc.size(),
            vertexAttributeDesc.data(),
        };
        Shit::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
            Shit::PrimitiveTopology::TRIANGLE_STRIP,
        };
        Shit::Viewport viewport{0,
                                0,
                                static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.width),
                                static_cast<float>(swapchain->GetCreateInfoPtr()->imageExtent.height),
                                0,
                                1};
        Shit::Rect2D scissor{{0, 0}, swapchain->GetCreateInfoPtr()->imageExtent};
        Shit::PipelineViewportStateCreateInfo viewportStateCreateInfo{1, &viewport, 1, &scissor};
        Shit::PipelineTessellationStateCreateInfo tessellationState{};
        Shit::PipelineRasterizationStateCreateInfo rasterizationState{
            false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false,
        };
        rasterizationState.lineWidth = 1.f;

        Shit::PipelineMultisampleStateCreateInfo multisampleState{
            Shit::SampleCountFlagBits::BIT_1,
        };
        Shit::PipelineDepthStencilStateCreateInfo depthStencilState{};

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentstate{};
        colorBlendAttachmentstate.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;
        Shit::PipelineColorBlendStateCreateInfo colorBlendState{
            false,
            {},
            1,
            &colorBlendAttachmentstate,
        };
        Shit::DynamicState dynamicStates[]{
            Shit::DynamicState::VIEWPORT,
            Shit::DynamicState::SCISSOR,
        };

        Shit::PipelineDynamicStateCreateInfo dynamicStateCI{std::size(dynamicStates), dynamicStates};

        Shit::GraphicsPipelineCreateInfo pipelineCreateInfo{shaderStageCreateInfos.size(),
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
                                                            0};

        pipeline = device->Create(pipelineCreateInfo);
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
    void createCommandBuffers() {
        Shit::CommandBufferCreateInfo cmdBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, MAX_FRAMES_IN_FLIGHT};
        commandBuffers.resize(cmdBufferCreateInfo.count);
        commandPool->CreateCommandBuffers(cmdBufferCreateInfo, commandBuffers.data());
    }
    void recordCommandBuffer(Shit::CommandBuffer *cmdBuffer, uint32_t frameIndex) {
        Shit::CommandBufferBeginInfo cmdBufferBeginInfo{};

        std::vector<Shit::ClearValue> clearValues{std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f}};

        Shit::BeginRenderPassInfo renderPassBeginInfo{renderPass,
                                                      nullptr,
                                                      Shit::Rect2D{{}, swapchain->GetCreateInfoPtr()->imageExtent},
                                                      static_cast<uint32_t>(clearValues.size()),
                                                      clearValues.data(),
                                                      Shit::SubpassContents::INLINE};

        float time = std::chrono::duration<float, std::chrono::seconds::period>(curTime - startTime).count();
        UBO_MVP uboMVP{
            glm::perspective(glm::radians(45.f),
                             float(swapchain->GetCreateInfoPtr()->imageExtent.width) /
                                 float(swapchain->GetCreateInfoPtr()->imageExtent.height),
                             1.f, 10.f) *
                glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0), glm::vec3(0, 1, 0)) *
                glm::rotate(glm::mat4(1), time * glm::radians(30.f), glm::vec3(0, 0, 1)),
        };

        auto &&ext = swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{0, 0}, {ext.width, ext.height}};

        renderPassBeginInfo.pFramebuffer = framebuffers[frameIndex];
        cmdBuffer->Reset();
        cmdBuffer->Begin(cmdBufferBeginInfo);
        cmdBuffer->BeginRenderPass(renderPassBeginInfo);
        cmdBuffer->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, pipeline});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        cmdBuffer->PushConstants(Shit::PushConstantInfo{pipelineLayout, Shit::ShaderStageFlagBits::VERTEX_BIT, 0,
                                                        sizeof(float) * 16, &uboMVP});

        uint64_t offsets[] = {0};
        Shit::Buffer *buffers[] = {vertexBuffer};
        cmdBuffer->BindVertexBuffers(Shit::BindVertexBuffersInfo{0, 1, buffers, offsets});

        Shit::BindDescriptorSetsInfo info{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1,
                                          &descriptorSets[frameIndex]};
        cmdBuffer->BindDescriptorSets(info);

        int drawMethod = 4;
        switch (drawMethod) {
            case 0:
            default: {
                Shit::DrawIndirectCommand drawIndirectCmd{4, 1, 0, 0};
                cmdBuffer->Draw(drawIndirectCmd);
            } break;
            case 1: {
                Shit::DrawIndirectInfo drawCmdInfo{drawIndirectCmdBuffer, 0, 1, sizeof(Shit::DrawIndirectCommand)};
                cmdBuffer->DrawIndirect(drawCmdInfo);
            } break;
            case 2: {
                Shit::DrawIndirectCountInfo drawCmdInfo{
                    drawIndirectCmdBuffer, 0, drawCountBuffer, 0, 1, sizeof(Shit::DrawIndirectCommand)};
                cmdBuffer->DrawIndirectCount(drawCmdInfo);
            } break;
            case 3: {
                // draw index
                cmdBuffer->BindIndexBuffer(Shit::BindIndexBufferInfo{indexBuffer, 0, Shit::IndexType::UINT16});
                Shit::DrawIndexedIndirectCommand drawIndexedIndirectCmd{4, 1, 0, 0, 0};
                cmdBuffer->DrawIndexed(drawIndexedIndirectCmd);
            } break;
            case 4: {
                // draw index
                cmdBuffer->BindIndexBuffer(Shit::BindIndexBufferInfo{indexBuffer, 0, Shit::IndexType::UINT16});
                cmdBuffer->DrawIndexedIndirect(Shit::DrawIndirectInfo{drawIndexedIndirectCmdBuffer, 0, 1,
                                                                      sizeof(Shit::DrawIndexedIndirectCommand)});
            } break;
            case 5: {
                // draw index
                cmdBuffer->BindIndexBuffer(Shit::BindIndexBufferInfo{indexBuffer, 0, Shit::IndexType::UINT16});
                cmdBuffer->DrawIndexedIndirectCount(Shit::DrawIndirectCountInfo{
                    drawIndexedIndirectCmdBuffer, 0, drawCountBuffer, 0, 1, sizeof(Shit::DrawIndexedIndirectCommand)});
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
        // inFlightImages.resize(swapchainImages.size());
    }
    void createDrawCommandBuffers() {
        // create draw indirect command buffer
        std::vector<Shit::DrawIndirectCommand> drawIndirectCmds{{4, 1, 0, 0}};
        Shit::BufferCreateInfo bufferCreateInfo{
            {},
            sizeof(Shit::DrawIndirectCommand) * drawIndirectCmds.size(),
            Shit::BufferUsageFlagBits::INDIRECT_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        drawIndirectCmdBuffer = device->Create(bufferCreateInfo, drawIndirectCmds.data());
        bufferCreateInfo.size = sizeof(uint32_t);
        uint32_t drawCount = 1;
        drawCountBuffer = device->Create(bufferCreateInfo, &drawCount);

        std::vector<Shit::DrawIndexedIndirectCommand> drawIndexedIndirectCmds{{4, 1, 0, 0, 0}};

        // create draw indexed indirect command buffer
        Shit::BufferCreateInfo drawIndexedIndirectCmdBufferCreateInfo{
            {},
            sizeof(Shit::DrawIndexedIndirectCommand) * drawIndexedIndirectCmds.size(),
            Shit::BufferUsageFlagBits::INDIRECT_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        drawIndexedIndirectCmdBuffer =
            device->Create(drawIndexedIndirectCmdBufferCreateInfo, drawIndexedIndirectCmds.data());
    }
    void createVertexBuffer() {
        Shit::BufferCreateInfo createInfo{
            {},
            sizeof(vertices[0]) * vertices.size(),
            Shit::BufferUsageFlagBits::VERTEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

        vertexBuffer = device->Create(createInfo, vertices.data());
    }
    void createIndexBuffer() {
        Shit::BufferCreateInfo indexBufferCreateInfo{
            {},
            sizeof(indices[0]) * indices.size(),
            Shit::BufferUsageFlagBits::INDEX_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        indexBuffer = device->Create(indexBufferCreateInfo, indices.data());
    }
    void createImages() {
        int width, height, components;
        auto pixels = stbi_load(testImagePath, &width, &height, &components,
                                4);  // force load an alpha channel,even not exist
        if (!pixels) throw std::runtime_error("failed to load texture image!");
        // mipmapLevels =
        // static_cast<uint32_t>(std::floor(std::log2((std::max)(width, height))) +
        // 1);

        Shit::ImageCreateInfo imageCreateInfo{
            .imageType = Shit::ImageType::TYPE_2D,
            .format = Shit::Format::R8G8B8A8_SRGB,
            .extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = Shit::SampleCountFlagBits::BIT_1,
            .tiling = Shit::ImageTiling::OPTIMAL,
            .usageFlags = Shit::ImageUsageFlagBits::TRANSFER_DST_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
            .memoryPropertyFlags = Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            .initialLayout = Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

        testImage = device->Create(imageCreateInfo, Shit::ImageAspectFlagBits::COLOR_BIT, pixels);

        stbi_image_free(pixels);

        Shit::ImageViewCreateInfo imageViewCreateInfo{
            .pImage = testImage,
            .viewType = Shit::ImageViewType::TYPE_2D,
            .format = Shit::Format::R8G8B8A8_SRGB,
            .subresourceRange = {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1},
        };
        testImageView = device->Create(imageViewCreateInfo);
    }
    void createSamplers() {
        Shit::SamplerCreateInfo samplerCreateInfo{
            .magFilter = Shit::Filter::NEAREST,
            .minFilter = Shit::Filter::NEAREST,
            .mipmapMode = Shit::SamplerMipmapMode::NEAREST,
            .wrapModeU = Shit::SamplerWrapMode::REPEAT,
            .wrapModeV = Shit::SamplerWrapMode::REPEAT,
            .wrapModeW = Shit::SamplerWrapMode::REPEAT,
        };
        sampler = device->Create(samplerCreateInfo);
    }
};
EXAMPLE_MAIN(Hello)
