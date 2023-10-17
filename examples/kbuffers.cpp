#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath0 = "kbuffers/write.vert";
static auto fragShaderPath0 = "kbuffers/write.frag";
static auto vertShaderPath1 = "kbuffers/read.vert";
static auto fragShaderPath1 = "kbuffers/read.frag";

static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj";
static auto imagePath = SHIT_SOURCE_DIR "/assets/images/limpopo_golf_course_4k.hdr";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    Shit::PipelineLayout *pipelineLayout;
    Shit::Pipeline *pipeline;
    std::vector<Shit::PipelineShaderStageCreateInfo> getStageCIs() {
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }
        return shaderStageCIs;
    }
};

class ModelController : public Behaviour {
    float _anglePerSecond = 3;

    void updatePerFrameImpl(float frameDtMs) override {
        getParentTransform()->yaw(glm::radians(frameDtMs / 1000 * _anglePerSecond), TransformSpace::PARENT);
    }

public:
    ModelController(GameObject *parent) : Behaviour(parent) {}
    constexpr void setSpeed(float anglePerSecond) { _anglePerSecond = anglePerSecond; }
    constexpr float getSpeed() const { return _anglePerSecond; }
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo0;
    PipelineInfo pipelineInfo1;

    RenderTarget skyboxRenderTarget;

    Image *eqImage;

    GameObject *modelObject;
    ModelController *modelController;

    std::vector<Shit::Image *> counterImages;
    std::vector<Shit::ImageView *> counterImageViews;
    std::vector<Shit::Image *> outImages;
    std::vector<Shit::ImageView *> outImageViews;

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        // modelController = modelObject->addComponent<ModelController>();
        _editorCamera->setFarPlane(10);
        modelController = _editorCamera->getParent()->addComponent<ModelController>();

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
        }

        eqImage = static_cast<Image *>(_imageManager->createOrRetrieve(imagePath));

        _environment = std::make_unique<Environment>(eqImage);
        _environment->prepare();

        //
        auto frameCount = getFramesInFlight();
        auto ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        auto listBufferSize = ext.width * ext.height * 128 * 2;
        for (uint32_t i = 0; i < frameCount; ++i) {
            auto image = counterImages.emplace_back(_device->Create(
                Shit::ImageCreateInfo{
                    {},
                    Shit::ImageType::TYPE_2D,
                    Shit::Format::R32_UINT,
                    {ext.width, ext.height, 1},
                    1,
                    1,
                    Shit::SampleCountFlagBits::BIT_1,
                    Shit::ImageTiling::OPTIMAL,
                    Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                    Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                    Shit::ImageLayout::GENERAL},
                Shit::ImageAspectFlagBits::COLOR_BIT, 0));
            counterImageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{image,
                                                          Shit::ImageViewType::TYPE_2D,
                                                          Shit::Format::R32_UINT,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}));

            image = outImages.emplace_back(_device->Create(
                Shit::ImageCreateInfo{
                    {},
                    Shit::ImageType::TYPE_2D,
                    Shit::Format::R32G32_UINT,
                    {ext.width, ext.height, 1},
                    1,
                    16,
                    Shit::SampleCountFlagBits::BIT_1,
                    Shit::ImageTiling::OPTIMAL,
                    Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                    Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                    Shit::ImageLayout::GENERAL},
                Shit::ImageAspectFlagBits::COLOR_BIT, 0));
            outImageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{image,
                                                          Shit::ImageViewType::TYPE_2D_ARRAY,
                                                          Shit::Format::R32G32_UINT,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 16}}));
        }
    }
    void createSkyboxRenderPass() {
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            {_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::LOAD,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference colAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        Shit::SubpassDescription subpassDescs[]{{Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &colAttachment}};

        skyboxRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(),
            attachmentDesc.data(),
            (uint32_t)std::size(subpassDescs),
            subpassDescs,
        });
        skyboxRenderTarget.clearValues = {Shit::ClearColorValueFloat{}};
    }
    void createSkyboxRenderTarget() {
        auto frameCount = _swapchain->GetImageCount();
        for (uint32_t i = 0; i < frameCount; ++i) {
            auto pImageView = skyboxRenderTarget.imageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{_swapchain->GetImageByIndex(i),
                                                          Shit::ImageViewType::TYPE_2D,
                                                          _swapchain->GetCreateInfoPtr()->format,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}));

            skyboxRenderTarget.frameBuffers.emplace_back(_device->Create(Shit::FramebufferCreateInfo{
                skyboxRenderTarget.renderPass, 1, &pImageView, _swapchain->GetCreateInfoPtr()->imageExtent, 1}));
        }
    }

    void createDefaultRenderPass() override {
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);
        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // color
            {_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference colorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDesc[]{
            {Shit::PipelineBindPoint::GRAPHICS},
            {Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &colorAttachment},
        };
        Shit::SubpassDependency subpassDependencies[]{
            {ST_SUBPASS_EXTERNAL,
             0,
             Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
             Shit::PipelineStageFlagBits::ALL_GRAPHICS_BIT,
             {},
             Shit::AccessFlagBits::MEMORY_READ_BIT | Shit::AccessFlagBits::INPUT_ATTACHMENT_READ_BIT |
                 Shit::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT | Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
            {0, 1, Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT, Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
             Shit::AccessFlagBits::MEMORY_WRITE_BIT, Shit::AccessFlagBits::MEMORY_READ_BIT},
            {1, ST_SUBPASS_EXTERNAL, Shit::PipelineStageFlagBits::ALL_GRAPHICS_BIT,
             Shit::PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT,
             Shit::AccessFlagBits::INPUT_ATTACHMENT_READ_BIT | Shit::AccessFlagBits::COLOR_ATTACHMENT_READ_BIT |
                 Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT}};

        _defaultRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(), attachmentDesc.data(), (uint32_t)std::size(subpassDesc), subpassDesc,
            (uint32_t)std::size(subpassDependencies), subpassDependencies});

        _defaultRenderTarget.clearValues = {
            Shit::ClearColorValueFloat{0, 0, 0, 0},
        };
    }
    void createDefaultRenderTarget() override {
        Shit::FramebufferCreateInfo frameBufferCI{
            _defaultRenderTarget.renderPass, _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount, 0,
            Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                           _swapchain->GetCreateInfoPtr()->imageExtent.height},
            1};

        std::vector<Shit::ImageView *> attachments;
        for (int i = 0; i < _swapchain->GetImageCount(); ++i) {
            _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            frameBufferCI.pAttachments = &_defaultRenderTarget.imageViews[i * frameBufferCI.attachmentCount];
            _defaultRenderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
        }
    }

    void preparePipelineInfos() {
        auto frameCount = getFramesInFlight();
        {
            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
                {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto setLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});
            std::vector<Shit::DescriptorSetLayout *> tempSetLayouts(frameCount, setLayout);

            // pipelineInfo 0
            pipelineInfo0.descriptorSets.resize(frameCount);
            _descriptorPool->Allocate({(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
                                      pipelineInfo0.descriptorSets.data());
            std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
                _transformDescriptorSetLayout,
                _cameraDescriptorSetLayout,
                _materialDescriptorSetLayout,
                _lightDescriptorSetLayout,
                setLayout,
            };

            pipelineInfo0.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

            //===============================
            pipelineInfo0.shaders = {static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, vertShaderPath0, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                     static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, fragShaderPath0, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};
        }
        {
            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
                {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto setLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});
            std::vector<Shit::DescriptorSetLayout *> tempSetLayouts(frameCount, setLayout);

            // pipelineInfo 1
            pipelineInfo1.descriptorSets.resize(frameCount);
            _descriptorPool->Allocate({(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
                                      pipelineInfo1.descriptorSets.data());

            std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
                setLayout,
            };

            pipelineInfo1.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

            //===============================
            pipelineInfo1.shaders = {static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, vertShaderPath1, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                     static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, fragShaderPath1, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};
        }
    }
    void createPipelineInfo0() {
        auto shaderStageCIs = pipelineInfo0.getStageCIs();

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

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        pipelineInfo0.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, Shit::CullMode::NONE, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
             0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                Shit::SampleCountFlagBits::BIT_1,
            },   // PipelineMultisampleStateCreateInfo
            {},  // PipelineDepthStencilStateCreateInfo
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
            pipelineInfo0.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void createPipelineInfo1() {
        auto shaderStageCIs = pipelineInfo1.getStageCIs();

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        pipelineInfo1.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            {},
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
             0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                Shit::SampleCountFlagBits::BIT_1,
            },   // PipelineMultisampleStateCreateInfo
            {},  // PipelineDepthStencilStateCreateInfo
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
            pipelineInfo1.pipelineLayout,
            _defaultRenderTarget.renderPass,
            1});
    }
    void updateDescriptorSets() {
        auto frameCount = getFramesInFlight();
        auto attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;

        std::vector<Shit::DescriptorImageInfo> imagesInfo(frameCount * 2);
        std::vector<Shit::WriteDescriptorSet> writes;

        for (uint32_t i = 0; i < frameCount; ++i) {
            imagesInfo[i * 2 + 0] = {0, counterImageViews[i], Shit::ImageLayout::GENERAL};
            imagesInfo[i * 2 + 1] = {0, outImageViews[i], Shit::ImageLayout::GENERAL};

            writes.emplace_back(Shit::WriteDescriptorSet{pipelineInfo0.descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[i * 2]});
            writes.emplace_back(Shit::WriteDescriptorSet{pipelineInfo0.descriptorSets[i], 1, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[i * 2 + 1]});

            writes.emplace_back(Shit::WriteDescriptorSet{pipelineInfo1.descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[i * 2]});
            writes.emplace_back(Shit::WriteDescriptorSet{pipelineInfo1.descriptorSets[i], 1, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[i * 2 + 1]});
        }
        _device->UpdateDescriptorSets(writes);
    }
    void drawGameObject(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                        Shit::PipelineLayout const *pipelineLayout, uint32_t materialSetNumber = ~(0u)) {
        if (auto meshRenderer = gameObject->getComponent<MeshRenderer>()) {
            auto descriptorSet = gameObject->getComponent<Transform>()->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(
                Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1, &descriptorSet});
            meshRenderer->draw(cmdBuffer, pipelineLayout, materialSetNumber);
        }

        auto it = gameObject->childBegin();
        auto end = gameObject->childEnd();
        for (; it != end; ++it) {
            drawGameObject(cmdBuffer, it->get(), pipelineLayout, materialSetNumber);
        }
    }
    void recordDefaultCommandBuffer(uint32_t imageIndex) override {
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{}, ext};

        auto frameCount = getFramesInFlight();

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
        auto pipeline0 = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo0.pipeline);
        auto pipeline1 = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo1.pipeline);

        Shit::ImageSubresourceRange range{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1};
        cmdBuffer->ClearColorImage(
            {counterImages[_currentFrame], Shit::ImageLayout::GENERAL, Shit::ClearColorValueUint32{0}, 1, &range});

        auto cameraDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);

        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        // bind camera
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline0->GetCreateInfoPtr()->pLayout, 1, 1, &cameraDescriptorSet});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline0});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline0->GetCreateInfoPtr()->pLayout, 4, 1,
                                                                   &pipelineInfo0.descriptorSets[_currentFrame]});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline0->GetCreateInfoPtr()->pLayout, 2);

        cmdBuffer->NextSubpass(Shit::SubpassContents::INLINE);
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline1});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline1->GetCreateInfoPtr()->pLayout, 0, 1,
                                                                   &pipelineInfo1.descriptorSets[_currentFrame]});

        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
        cmdBuffer->EndRenderPass();

        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{skyboxRenderTarget.renderPass,
                                                             skyboxRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)skyboxRenderTarget.clearValues.size(),
                                                             skyboxRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        _environment->draw(cmdBuffer, cameraDescriptorSet);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("settings");
        static float rotateSpeed = modelController->getSpeed();
        if (ImGui::DragFloat("rotate speed", &rotateSpeed)) {
            modelController->setSpeed(rotateSpeed);
        }
        static bool pause = false;
        if (ImGui::Checkbox("pause", &pause)) {
            if (pause)
                modelController->pause();
            else
                modelController->run();
        }
        ImGui::End();
    }
    void recreateSwapchainListener() {
        auto frameCount = getFramesInFlight();
        auto ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        for (uint32_t i = 0; i < frameCount; ++i) {
            _device->Destroy(counterImages[i]);
            _device->Destroy(counterImageViews[i]);
            _device->Destroy(outImages[i]);
            _device->Destroy(outImageViews[i]);

            counterImages[i] = _device->Create(Shit::ImageCreateInfo{{},
                                                                     Shit::ImageType::TYPE_2D,
                                                                     Shit::Format::R32_UINT,
                                                                     {ext.width, ext.height, 1},
                                                                     1,
                                                                     1,
                                                                     Shit::SampleCountFlagBits::BIT_1,
                                                                     Shit::ImageTiling::OPTIMAL,
                                                                     Shit::ImageUsageFlagBits::STORAGE_BIT |
                                                                         Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                                                                     Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                     Shit::ImageLayout::GENERAL},
                                               Shit::ImageAspectFlagBits::COLOR_BIT, -1);
            counterImageViews[i] =
                _device->Create(Shit::ImageViewCreateInfo{counterImages[i],
                                                          Shit::ImageViewType::TYPE_2D,
                                                          Shit::Format::R32_UINT,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}});

            outImages[i] = _device->Create(Shit::ImageCreateInfo{{},
                                                                 Shit::ImageType::TYPE_2D,
                                                                 Shit::Format::R32G32_UINT,
                                                                 {ext.width, ext.height, 1},
                                                                 1,
                                                                 16,
                                                                 Shit::SampleCountFlagBits::BIT_1,
                                                                 Shit::ImageTiling::OPTIMAL,
                                                                 Shit::ImageUsageFlagBits::STORAGE_BIT |
                                                                     Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                                                                 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                 Shit::ImageLayout::GENERAL},
                                           Shit::ImageAspectFlagBits::COLOR_BIT, 0);
            outImageViews[i] =
                _device->Create(Shit::ImageViewCreateInfo{outImages[i],
                                                          Shit::ImageViewType::TYPE_2D_ARRAY,
                                                          Shit::Format::R32G32_UINT,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 16}});
        }
        for (auto e : skyboxRenderTarget.imageViews) _device->Destroy(e);
        skyboxRenderTarget.imageViews.clear();
        for (auto e : skyboxRenderTarget.frameBuffers) _device->Destroy(e);
        skyboxRenderTarget.frameBuffers.clear();
        createSkyboxRenderTarget();
        updateDescriptorSets();
    }
    void prepare() override {
        loadAssets();
        createSkyboxRenderPass();
        createSkyboxRenderTarget();

        preparePipelineInfos();
        createPipelineInfo0();
        createPipelineInfo1();
        updateDescriptorSets();

        addRecreateSwapchainListener(std::bind(&Hello::recreateSwapchainListener, this));
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_1)