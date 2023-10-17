#include "common/entry.h"

constexpr uint32_t WIDTH = 800, HEIGHT = 600;

std::string vertShaderPath;
std::string fragShaderPath;

std::vector<uint16_t> indices{0, 1, 3, 0, 2, 3};

class Hello {
    Shit::RenderSystem *renderSystem;
    Shit::Device *device;

    std::vector<Shit::Image *> offscreenImages;
    std::vector<Shit::ImageView *> offscreenImageViews;
    std::vector<Shit::Framebuffer *> framebuffers;

    Shit::CommandPool *commandPool;
    std::vector<Shit::CommandBuffer *> commandBuffers;
    Shit::Queue *graphicsQueue;
    Shit::Semaphore *imageAvailableSemaphore;
    Shit::Semaphore *renderFinishedSemaphore;

    Shit::Shader *vertShader;
    Shit::Shader *fragShader;

    Shit::PipelineLayout *pipelineLayout;
    Shit::RenderPass *renderPass;
    Shit::Pipeline *pipeline;

    Shit::Buffer *drawIndirectCmdBuffer;
    Shit::Buffer *drawCountBuffer;
    Shit::Buffer *drawIndexedIndirectCmdBuffer;
    Shit::Buffer *indexBuffer;

public:
    void initRenderSystem() {
        Shit::RenderSystemCreateInfo renderSystemCreateInfo{g_RendererVersion,
                                                            Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT};

        renderSystem = LoadRenderSystem(renderSystemCreateInfo);

        device = renderSystem->CreateDevice({});

        // 3
        auto graphicsQueueFamilyProperty = renderSystem->GetQueueFamily(Shit::QueueFlagBits::GRAPHICS_BIT);
        if (!graphicsQueueFamilyProperty.has_value()) THROW("failed to find a graphic queue")

        LOG_VAR(graphicsQueueFamilyProperty->index);
        LOG_VAR(graphicsQueueFamilyProperty->count);

        // 4. command pool
        Shit::CommandPoolCreateInfo commandPoolCreateInfo{{}, graphicsQueueFamilyProperty->index};
        commandPool = device->Create(commandPoolCreateInfo);

        // 5
        graphicsQueue = device->GetQueue(graphicsQueueFamilyProperty->index, 0);

        createShaders();
        createDrawCommandBuffers();
        createSyncObjects();
        createIndexBuffer();

        createOffscreenImages();
        createRenderPasses();
        createFramebuffers();
        createPipeline();
        createCommandBuffers();
    }

    void mainLoop() { drawFrame(); }
    void cleanUp() {}
    void run() {
        initRenderSystem();
        mainLoop();
        cleanUp();
    }
    void drawFrame() {
        static uint32_t imageIndex{};
        std::vector<Shit::SubmitInfo> submitInfos{
            {0, nullptr, 1, &commandBuffers[imageIndex], 1, &renderFinishedSemaphore}};

        graphicsQueue->Submit(submitInfos, nullptr);

        takeScreenshot(device, offscreenImages[imageIndex], Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL);
        graphicsQueue->WaitIdle();
    }

    void createShaders() {
        g_shaderSourceLanguage =
            chooseShaderShourceLanguage({Shit::ShadingLanguage::SPIRV, Shit::ShadingLanguage::GLSL}, device);

        std::string vertCode =
            "\
#version 460\n\
#ifdef VULKAN\n\
#define VertexID gl_VertexIndex\n\
#else\n\
#define VertexID gl_VertexID\n\
#endif\n\
const vec2 inPos[4] = {{-0.5, -0.5}, {0.5, -0.5},{-0.5, 0.5},{0.5,0.5}};\n\
const vec4 color[4]={{1,0,0,1},{0,1,0,1},{0,0,1,1},{1,1,1,1}};\n\
struct VS_OUT{\n\
	vec4 color;\n\
};\n\
layout(location=0) out VS_OUT vs_out;\n\
void main() { \n\
	gl_Position = vec4(inPos[VertexID], 0, 1); \n\
	vs_out.color=color[VertexID];\n\
}\n	";
        std::string fragCode =
            "\
#version 460\n\
layout(location = 0) out vec4 outColor;\n\
struct VS_OUT{\n\
	vec4 color;\n\
};\n\
layout(location=0)in VS_OUT fs_in;\n\
void main() { \n\
	outColor = fs_in.color;\n\
}\n";

        std::string vertCodeSpv =
            compileGlslToSpirv(vertCode, Shit::ShaderStageFlagBits::VERTEX_BIT, g_RendererVersion);
        std::string fragCodeSpv =
            compileGlslToSpirv(fragCode, Shit::ShaderStageFlagBits::FRAGMENT_BIT, g_RendererVersion);

        vertShader =
            device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, vertCodeSpv.size(), vertCodeSpv.data()});
        fragShader =
            device->Create(Shit::ShaderCreateInfo{g_shaderSourceLanguage, fragCodeSpv.size(), fragCodeSpv.data()});
    }

    void createRenderPasses() {
        Shit::AttachmentDescription attachmentDescriptions[]{
            {offscreenImages[0]->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1,
             Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE,
             Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED,  // inital layout
             Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            {offscreenImages[1]->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1,
             Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE,
             Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED,  // inital layout
             Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
        };

        Shit::AttachmentReference colorAttachments[]{
            {0,  // the index of attachment description
             Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference depthAttachment{1,  // the index of attachment description
                                                  Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subPasses[]{
            Shit::SubpassDescription{Shit::PipelineBindPoint::GRAPHICS, 0, nullptr, 1, colorAttachments, nullptr,
                                     &depthAttachment},
        };

        Shit::RenderPassCreateInfo renderPassCreateInfo{ARR_SIZE(attachmentDescriptions), attachmentDescriptions,
                                                        ARR_SIZE(subPasses), subPasses};

        renderPass = device->Create(renderPassCreateInfo);
    }
    void createPipeline() {
        pipelineLayout = device->Create(Shit::PipelineLayoutCreateInfo{});
        Shit::PipelineShaderStageCreateInfo shaderStageCreateInfos[]{
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
        Shit::VertexInputStateCreateInfo vertexInputState{};
        Shit::PipelineInputAssemblyStateCreateInfo inputAssemblyState{
            Shit::PrimitiveTopology::TRIANGLE_LIST,
        };
        Shit::Viewport viewport{0, 0, float(WIDTH), float(HEIGHT), 0, 1};
        Shit::Rect2D scissor{{0, 0}, {WIDTH, HEIGHT}};
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

        Shit::GraphicsPipelineCreateInfo pipelineCreateInfo{ARR_SIZE(shaderStageCreateInfos),
                                                            shaderStageCreateInfos,
                                                            vertexInputState,
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
    }
    void createOffscreenImages() {
        Shit::Format formats[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = device->GetSuitableImageFormat(formats, Shit::ImageTiling::OPTIMAL,
                                                          Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        offscreenImages = {
            device->Create(Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                // Shit::ShitFormat::B8G8R8A8_SRGB,
                Shit::Format::R8G8B8A8_SRGB,
                {WIDTH, HEIGHT, 1},
                1,
                1,
                Shit::SampleCountFlagBits::BIT_1,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            }),
            device->Create(Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                depthFormat,
                {WIDTH, HEIGHT, 1},
                1,
                1,
                Shit::SampleCountFlagBits::BIT_1,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            }),
        };

        offscreenImageViews.resize(offscreenImages.size());
        offscreenImageViews[0] =
            device->Create(Shit::ImageViewCreateInfo{offscreenImages[0],
                                                     Shit::ImageViewType::TYPE_2D,
                                                     offscreenImages[0]->GetCreateInfoPtr()->format,
                                                     {},
                                                     {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});
        offscreenImageViews[1] = device->Create(Shit::ImageViewCreateInfo{
            offscreenImages[1],
            Shit::ImageViewType::TYPE_2D,
            offscreenImages[1]->GetCreateInfoPtr()->format,
            {},
            {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}});
    }
    void createFramebuffers() {
        Shit::FramebufferCreateInfo framebufferCreateInfo{
            renderPass, (uint32_t)offscreenImageViews.size(), offscreenImageViews.data(), {WIDTH, HEIGHT}, 1};
        framebuffers.resize(1);
        framebuffers[0] = device->Create(framebufferCreateInfo);
    }
    void createCommandBuffers() {
        uint32_t count = 1;
        Shit::CommandBufferCreateInfo cmdBufferCreateInfo{Shit::CommandBufferLevel::PRIMARY, count};

        commandBuffers.resize(count);
        commandPool->CreateCommandBuffers(cmdBufferCreateInfo, commandBuffers.data());

        Shit::CommandBufferBeginInfo cmdBufferBeginInfo{};

        std::vector<Shit::ClearValue> clearValues{std::array<float, 4>{0.2f, 0.2f, 0.2f, 1.f},
                                                  Shit::ClearDepthStencilValue{1, 0}};

        Shit::BeginRenderPassInfo renderPassBeginInfo{renderPass,
                                                      nullptr,
                                                      Shit::Rect2D{{}, {WIDTH, HEIGHT}},
                                                      static_cast<uint32_t>(clearValues.size()),
                                                      clearValues.data(),
                                                      Shit::SubpassContents::INLINE};

        for (uint32_t i = 0; i < count; ++i) {
            renderPassBeginInfo.pFramebuffer = framebuffers[i];
            commandBuffers[i]->Begin(cmdBufferBeginInfo);
            commandBuffers[i]->BeginRenderPass(renderPassBeginInfo);
            commandBuffers[i]->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, pipeline});

            int drawMethod = 0;
            switch (drawMethod) {
                case 0:
                default: {
                    Shit::DrawIndirectCommand drawIndirectCmd{3, 1, 0, 0};
                    commandBuffers[i]->Draw(drawIndirectCmd);
                } break;
                case 1: {
                    Shit::DrawIndirectInfo drawCmdInfo{drawIndirectCmdBuffer, 0, 1, sizeof(Shit::DrawIndirectCommand)};
                    commandBuffers[i]->DrawIndirect(drawCmdInfo);
                } break;
                case 2: {
                    Shit::DrawIndirectCountInfo drawCmdInfo{
                        drawIndirectCmdBuffer, 0, drawCountBuffer, 0, 1, sizeof(Shit::DrawIndirectCommand)};
                    commandBuffers[i]->DrawIndirectCount(drawCmdInfo);
                } break;
                case 3: {
                    // draw index
                    commandBuffers[i]->BindIndexBuffer(
                        Shit::BindIndexBufferInfo{indexBuffer, 0, Shit::IndexType::UINT16});
                    Shit::DrawIndexedIndirectCommand drawIndexedIndirectCmd{static_cast<uint32_t>(indices.size()), 1, 0,
                                                                            0, 0};
                    commandBuffers[i]->DrawIndexed(drawIndexedIndirectCmd);
                } break;
            }
            commandBuffers[i]->EndRenderPass();
            commandBuffers[i]->End();
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphore = device->Create(Shit::SemaphoreCreateInfo{});
        renderFinishedSemaphore = device->Create(Shit::SemaphoreCreateInfo{});
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
};
EXAMPLE_MAIN(Hello)