#include "common/appbase.h"
#include "common/entry.h"
static auto writeVertShaderPath = "inputattachment/write.vert";
static auto writeFragShaderPath = "inputattachment/write.frag";
static auto readVertShaderPath = "inputattachment/read.vert";
static auto readFragShaderPath = "inputattachment/read.frag";
static auto readMultisampleFragShaderPath = "inputattachment/readmultisample.frag";

static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/sampleroom.obj";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
};

struct UBORead {
    glm::vec2 range;
    int attachmentIndex;
    int sampleCount;
};

class Hello : public AppBase {
    PipelineInfo writePipelineInfo;
    PipelineInfo readPipelineInfo;

    UBORead uboRead{};
    Shit::Buffer *uboReadBuffer;
    std::vector<Shit::DescriptorSet *> readDescriptorSets;

    std::vector<Shit::ImageView *> depthInputAttachmentImageView;

    GameObject *modelObject;
    Light *light;

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        // modelObject->getComponent<Transform>()
        //	->pitch(glm::radians(-90.f))
        //	->roll(glm::radians(-90.f));

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
        }
        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));
    }
    void createDefaultRenderPass() override {
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // read color
            {_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            // write
            // color
            {_swapchain->GetCreateInfoPtr()->format, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // depth
            {depthFormat, _sampleCount, Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
             Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE, Shit::ImageLayout::UNDEFINED,
             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::AttachmentReference colorAttachment{1, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        Shit::AttachmentReference depthAttachment{2, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::AttachmentReference inputAttachments1[]{
            {1, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {2, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::AttachmentReference colorAttachment1{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDescs[]{
            {Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &colorAttachment, 0, &depthAttachment},
            {Shit::PipelineBindPoint::GRAPHICS, std::size(inputAttachments1), inputAttachments1, 1, &colorAttachment1},
        };
        Shit::SubpassDependency subpassDependencies[]{
            {ST_SUBPASS_EXTERNAL,
             0,
             Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
             Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
                 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
             {},
             Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
            {0, 1,
             Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
                 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
             Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
             Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
             Shit::AccessFlagBits::SHADER_READ_BIT},
            {1, ST_SUBPASS_EXTERNAL, Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
             Shit::PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT, Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT}};

        _defaultRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(),
            attachmentDesc.data(),
            std::size(subpassDescs),
            subpassDescs,
            std::size(subpassDependencies),
            subpassDependencies,
        });

        Shit::ClearColorValueFloat clearColor = {0.2, 0.2, 0.2, 1};
        Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
        _defaultRenderTarget.clearValues = {clearColor, clearColor, clearDepthStencil};
    }
    void createDefaultRenderTarget() override {
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
            Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::INPUT_ATTACHMENT_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        Shit::ImageViewCreateInfo colorImageViewCI{0,
                                                   Shit::ImageViewType::TYPE_2D,
                                                   _swapchain->GetCreateInfoPtr()->format,
                                                   {},
                                                   {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

        auto attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        auto depthFormat =
            _defaultRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[attachmentCount - 1].format;

        //================
        Shit::ImageCreateInfo depthImageCI{
            {},  // create flags
            Shit::ImageType::TYPE_2D,
            depthFormat,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,  // mipmap levels,
            1,  // array layers
            _sampleCount,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::INPUT_ATTACHMENT_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

        Shit::ImageViewCreateInfo depthImageViewCI{
            0,
            Shit::ImageViewType::TYPE_2D,
            depthFormat,
            {},
            {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}};

        Shit::FramebufferCreateInfo frameBufferCI{
            _defaultRenderTarget.renderPass,
            0,
            0,
            Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                           _swapchain->GetCreateInfoPtr()->imageExtent.height},
            1,
        };

        for (int i = 0; i < _swapchain->GetImageCount(); ++i) {
            _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            colorImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(colorImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(colorImageViewCI));

            depthImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            frameBufferCI.attachmentCount = attachmentCount;
            frameBufferCI.pAttachments = &_defaultRenderTarget.imageViews[i * attachmentCount];
            _defaultRenderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
        }
    }
    void prepareWritePipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };
        auto pipelineLayoutCI =
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};

        auto pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        writePipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, writeVertShaderPath, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                     static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, writeFragShaderPath, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : writePipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
            {1, (uint32_t)sizeof(float) * 3, 0},
            {2, (uint32_t)sizeof(float) * 2, 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
            {1, 1, Shit::Format::R32G32B32_SFLOAT, 0},
            {2, 2, Shit::Format::R32G32_SFLOAT, 0},
        };
        Shit::VertexInputStateCreateInfo vertexInputStageCI{
            (uint32_t)std::size(vertexBindings),
            vertexBindings,
            (uint32_t)std::size(attributeBindings),
            attributeBindings,
        };

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        writePipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
             0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                _sampleCount,
            },  // PipelineMultisampleStateCreateInfo
            {
                true,
                true,
                Shit::CompareOp::LESS,
            },  // PipelineDepthStencilStateCreateInfo
            {
                false,
                {},
                1,
                &colorBlendAttachmentState,
            },  // PipelineColorBlendStateCreateInfo
            {
                (uint32_t)std::size(dynamicStates),
                dynamicStates,
            },  // PipelineDynamicStateCreateInfo
            pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void prepareReadPipeline() {
        // pipelina layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {1, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {2, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        Shit::DescriptorSetLayout *descriptorSetLayout =
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        auto count = _swapchain->GetImageCount();
        std::vector<Shit::DescriptorSetLayout const *> descriptorSetLayouts(count, descriptorSetLayout);

        readDescriptorSets.resize(descriptorSetLayouts.size());
        _descriptorPool->Allocate(
            Shit::DescriptorSetAllocateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()},
            readDescriptorSets.data());

        auto pipelineLayoutCI = Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout};
        auto pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        readPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, readVertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(
                buildShaderPath(_device,
                                _sampleCount == Shit::SampleCountFlagBits::BIT_1 ? readFragShaderPath
                                                                                 : readMultisampleFragShaderPath,
                                _rendererVersion),
                ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : readPipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
            {1, (uint32_t)sizeof(float) * 3, 0},
            {2, (uint32_t)sizeof(float) * 2, 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
            {1, 1, Shit::Format::R32G32B32_SFLOAT, 0},
            {2, 2, Shit::Format::R32G32_SFLOAT, 0},
        };
        Shit::VertexInputStateCreateInfo vertexInputStageCI{
            (uint32_t)std::size(vertexBindings),
            vertexBindings,
            (uint32_t)std::size(attributeBindings),
            attributeBindings,
        };

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        readPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
             0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                Shit::SampleCountFlagBits::BIT_1,
            },  // PipelineMultisampleStateCreateInfo
            {
                false,
            },  // PipelineDepthStencilStateCreateInfo
            {
                false,
                {},
                1,
                &colorBlendAttachmentState,
            },  // PipelineColorBlendStateCreateInfo
            {
                (uint32_t)std::size(dynamicStates),
                dynamicStates,
            },  // PipelineDynamicStateCreateInfo
            pipelineLayout,
            _defaultRenderTarget.renderPass,
            1});
    }
    void prepareDescriptorSets() {
        uboRead.range = {0, 1};
        uboRead.sampleCount = int(_sampleCount);
        uboReadBuffer = _device->Create(
            Shit::BufferCreateInfo{
                {},
                sizeof(UBORead),
                Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT},
            &uboRead);
        depthInputAttachmentImageView.resize(_swapchain->GetImageCount());
    }
    void updateDescriptorSets() {
        //
        auto count = _swapchain->GetImageCount();
        Shit::DescriptorBufferInfo bufferInfo{uboReadBuffer, 0, sizeof(UBORead)};
        std::vector<Shit::DescriptorImageInfo> imagesInfo(2 * count);

        std::vector<Shit::WriteDescriptorSet> writes;
        auto attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        // write
        for (uint32_t i = 0; i < count; ++i) {
            _device->Destroy(depthInputAttachmentImageView[i]);
            depthInputAttachmentImageView[i] = _device->Create(Shit::ImageViewCreateInfo{
                _defaultRenderTarget.imageViews[i * attachmentCount + 2]->GetCreateInfoPtr()->pImage,
                Shit::ImageViewType::TYPE_2D,
                _defaultRenderTarget.imageViews[i * attachmentCount + 2]->GetCreateInfoPtr()->format,
                {},
                {Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, 1}});

            imagesInfo[i] = Shit::DescriptorImageInfo{0, _defaultRenderTarget.imageViews[i * attachmentCount + 1],
                                                      Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            imagesInfo[i + count] = Shit::DescriptorImageInfo{0, depthInputAttachmentImageView[i],
                                                              Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            writes.emplace_back(Shit::WriteDescriptorSet{readDescriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::INPUT_ATTACHMENT, &imagesInfo[i]});
            writes.emplace_back(Shit::WriteDescriptorSet{
                readDescriptorSets[i], 1, 0, 1, Shit::DescriptorType::INPUT_ATTACHMENT, &imagesInfo[i + count]});
            writes.emplace_back(Shit::WriteDescriptorSet{readDescriptorSets[i], 2, 0, 1,
                                                         Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo});
        }
        _device->UpdateDescriptorSets(writes);
    }

    void drawGameObject(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                        Shit::PipelineLayout const *pipelineLayout) {
        if (auto meshRenderer = gameObject->getComponent<MeshRenderer>()) {
            auto descriptorSet = gameObject->getComponent<Transform>()->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(
                Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1, &descriptorSet});
            meshRenderer->draw(cmdBuffer, pipelineLayout, 2);
        }

        auto it = gameObject->childBegin();
        auto end = gameObject->childEnd();
        for (; it != end; ++it) {
            drawGameObject(cmdBuffer, it->get(), pipelineLayout);
        }
    }
    void recordDefaultCommandBuffer(uint32_t imageIndex) override {
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{}, ext};

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        auto writePipelineLayout =
            dynamic_cast<Shit::GraphicsPipeline *>(writePipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout;
        // bind camera
        auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, writePipelineLayout, 1, 1, &descriptorSet});

        // bind light
        descriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, writePipelineLayout, 3, 1, &descriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, writePipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), writePipelineLayout);

        cmdBuffer->NextSubpass(Shit::SubpassContents::INLINE);
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS,
            dynamic_cast<Shit::GraphicsPipeline *>(readPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout, 0, 1,
            &readDescriptorSets[imageIndex]});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("aaa");

        static const char *items[]{
            "color",
            "depth",
        };
        static bool flag = false;
        if (ImGui::BeginCombo("input attachment", items[uboRead.attachmentIndex])) {
            for (int i = 0; i < std::size(items); ++i) {
                const bool is_selected = uboRead.attachmentIndex == i;
                if (ImGui::Selectable(items[i], is_selected)) {
                    uboRead.attachmentIndex = i;
                    flag |= true;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::End();
        if (flag) {
            void *data;
            uboReadBuffer->MapMemory(0, sizeof(UBORead), &data);
            memcpy(data, &uboRead, sizeof(UBORead));
            uboReadBuffer->UnMapMemory();
            flag = false;
        }
    }
    void recreateSwapchainListener() { updateDescriptorSets(); }
    void prepare() override {
        loadAssets();
        prepareWritePipeline();
        prepareReadPipeline();
        prepareDescriptorSets();
        updateDescriptorSets();
        addRecreateSwapchainListener(std::bind(&Hello::recreateSwapchainListener, this));
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)