#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "octreetest/display.vert";
static auto fragShaderPath = "octreetest/display.frag";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/chinesedragon.obj";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/SheenCloth/SheenCloth.obj";
static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/teapot.obj";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/sphere_ico.obj"; static auto testModelPath =
// SHIT_SOURCE_DIR "/assets/models/obj/cornell_box2.obj";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    std::vector<Shit::DescriptorSet *> descriptorSets;
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo0;
    PipelineInfo pipelineInfo1;

    GameObject *modelObject;
    Light *light;

    PipelineInfo drawLinePipelineInfo;

    bool frameMode{0};
    int octreeMode{1};

    GameObject *octreePointsObject;
    GameObject *octreeFacesObject;

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath))
                         ->enableGenerateOctreePoints(9)
                         ->enableGenerateOctreeFaces(7);
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
            child->addComponent<MeshPointRenderer>(it->get());

            octreePointsObject = child->addChild();
            octreePointsObject->addComponent<OctreeMeshRenderer>((*it)->octreePoints.get());
            octreePointsObject->enable();
            octreeFacesObject = child->addChild();
            octreeFacesObject->addComponent<OctreeMeshRenderer>((*it)->octreeFaces.get());
            octreeFacesObject->disable();
        }
        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));
    }
    void preparePipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
        };

        Shit::PushConstantRange range{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(float) * 4};

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                                             descriptorSetLayouts.data(), 1, &range});

        //===============================
        pipelineInfo0.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, fragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : pipelineInfo0.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
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

        pipelineInfo0.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::POINT_LIST},  // PipelineInputAssemblyStateCreateInfo
            //{Shit::PrimitiveTopology::TRIANGLE_LIST}, //
            // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},  // PipelineViewportStateCreateInfo
            {},                           // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::POINT, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
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
    void preparePipeline1() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
        };

        Shit::PushConstantRange range{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(float) * 4};

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                                             descriptorSetLayouts.data(), 1, &range});

        //===============================
        pipelineInfo1.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, fragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : pipelineInfo1.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
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

        pipelineInfo1.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            //{Shit::PrimitiveTopology::TRIANGLE_LIST}, //
            // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},  // PipelineViewportStateCreateInfo
            {},                           // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::LINE, Shit::CullMode::NONE, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
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
    void createDrawLinePipelineInfo() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
        };
        Shit::PushConstantRange range{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(float) * 4};

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                                             descriptorSetLayouts.data(), 1, &range});

        //===============================
        drawLinePipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, fragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : drawLinePipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
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

        drawLinePipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::LINE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},           // PipelineViewportStateCreateInfo
            {},                                    // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::LINE, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
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

    void drawGameObject(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                        Shit::PipelineLayout const *pipelineLayout) {
        if (auto meshRenderer = gameObject->getComponent<MeshRenderer>()) {
            auto descriptorSet = gameObject->getComponent<Transform>()->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(
                Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1, &descriptorSet});
            meshRenderer->draw(cmdBuffer);
        }

        auto it = gameObject->childBegin();
        auto end = gameObject->childEnd();
        for (; it != end; ++it) {
            drawGameObject(cmdBuffer, it->get(), pipelineLayout);
        }
    }
    void drawGameObjectPoint(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                             Shit::PipelineLayout const *pipelineLayout) {
        if (auto meshRenderer = gameObject->getComponent<MeshPointRenderer>()) {
            auto descriptorSet = gameObject->getComponent<Transform>()->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(
                Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1, &descriptorSet});
            meshRenderer->draw(cmdBuffer);
        }

        auto it = gameObject->childBegin();
        auto end = gameObject->childEnd();
        for (; it != end; ++it) {
            drawGameObject(cmdBuffer, it->get(), pipelineLayout);
        }
    }
    void drawGameObjectOctree(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                              Shit::PipelineLayout const *pipelineLayout) {
        if (!gameObject->isEnable()) return;
        if (auto renderer = gameObject->getComponent<OctreeMeshRenderer>()) {
            auto descriptorSet = gameObject->getComponent<Transform>()->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(
                Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineLayout, 0, 1, &descriptorSet});
            renderer->draw(cmdBuffer);
        }

        auto it = gameObject->childBegin();
        auto end = gameObject->childEnd();
        for (; it != end; ++it) {
            drawGameObjectOctree(cmdBuffer, it->get(), pipelineLayout);
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

        Shit::GraphicsPipeline *pipeline;
        if (frameMode)
            pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo1.pipeline);
        else
            pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo0.pipeline);

        // bind camera
        auto cameraDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &cameraDescriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline});

        float color[4]{1, 1, 1, 1};
        cmdBuffer->PushConstants({pipeline->GetCreateInfoPtr()->pLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0,
                                  sizeof(float) * 4, color});

        for (auto &&e : _gameObjects) {
            if (frameMode)
                drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);
            else
                drawGameObjectPoint(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);
        }

        if (octreeMode != 0) {
            pipeline = dynamic_cast<Shit::GraphicsPipeline *>(drawLinePipelineInfo.pipeline);
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, drawLinePipelineInfo.pipeline});

            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &cameraDescriptorSet});

            color[0] = 0;
            color[2] = 0;
            cmdBuffer->PushConstants({pipeline->GetCreateInfoPtr()->pLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0,
                                      sizeof(float) * 4, color});
            for (auto &&e : _gameObjects)
                drawGameObjectOctree(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);
        }

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("settings");

        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("frameMode", &frameMode);

        static const char *items[]{
            "none",
            "octree points",
            "octree face",
        };
        const char *mode = items[octreeMode];  // Label to preview before opening the combo
                                               // (technically it could be anything)
        if (ImGui::BeginCombo("octreeMode", mode)) {
            for (int i = 0; i < std::size(items); ++i) {
                const bool is_selected = octreeMode == i;
                if (ImGui::Selectable(items[i], is_selected)) {
                    octreeMode = i;
                    _needUpdateDefaultCommandBuffer = true;
                    switch (octreeMode) {
                        case 0:
                            octreePointsObject->disable();
                            octreeFacesObject->disable();
                            break;
                        case 1:
                            octreePointsObject->enable();
                            octreeFacesObject->disable();
                            break;
                        case 2:
                            octreePointsObject->disable();
                            octreeFacesObject->enable();
                            break;
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::End();
    }

    void prepare() override {
        loadAssets();
        preparePipeline();
        preparePipeline1();
        createDrawLinePipelineInfo();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_1)