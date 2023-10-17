#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "arealight/display.vert";
static auto fragShaderPath = "arealight/display.frag";
static auto showLightFragShaderPath = "arealight/color.frag";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj";
static auto sphereModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj";
static auto diskModelPath = SHIT_SOURCE_DIR "/assets/models/obj/disk.obj";
static auto quadModelPath = SHIT_SOURCE_DIR "/assets/models/obj/plane.obj";
static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sampleroom.obj";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    Shit::PipelineLayout *pipelineLayout;
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;
    PipelineInfo showLightPipelineInfo;
    GameObject *modelObject;
    Light *sphereLight;
    Light *tubeLight;
    Light *diskLight;
    Light *quadLight;
    Light *curLight{};

public:
    GameObject *loadModel(char const *path = nullptr) {
        auto ret = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        if (path) {
            auto model = static_cast<Model *>(_modelManager->create(path));
            model->load();
            for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
                ret->addChild()->addComponent<MeshRenderer>(it->get());
            }
        }
        return ret;
    }
    void loadAssets() {
        modelObject = loadModel(testModelPath);
        modelObject->getComponent<Transform>()->scale({10, 10, 10})->rotate({0, glm::radians(90.f), 0});

        auto sphere = loadModel(sphereModelPath);
        auto materialDatablock =
            (*sphere->childBegin())->getComponent<MeshRenderer>()->getPrimitiveRenderer(0)->getMaterialDataBlock();
        float emission[3]{1, 1, 1};
        memcpy(materialDatablock->getUBOData()->emission, emission, sizeof(float) * 3);
        memset(materialDatablock->getUBOData()->diffuse, 0, sizeof(float) * 3);

        sphereLight = sphere->addComponent<Light>(LIGHT_SPHERE);
        sphereLight->getParentTransform()->setLocalTranslation({0, 50, 0})->setLocalScale({5, 5, 5});
        sphereLight->getSSBOData()->radiance = 50;
        sphereLight->needUpdate();

        auto tube = loadModel();
        tubeLight = tube->addComponent<Light>(LIGHT_TUBE);
        tubeLight->getParentTransform()->setLocalTranslation({-20, 50, 0})->scale({50, 1, 1});
        tubeLight->getSSBOData()->radiance = 30;
        tubeLight->needUpdate();

        //=====
        auto disk = loadModel(diskModelPath);
        disk->getComponent<Transform>()->scale({30, 1, 30});
        materialDatablock =
            (*disk->childBegin())->getComponent<MeshRenderer>()->getPrimitiveRenderer(0)->getMaterialDataBlock();
        memcpy(materialDatablock->getUBOData()->emission, emission, sizeof(float) * 3);
        memset(materialDatablock->getUBOData()->diffuse, 0, sizeof(float) * 3);
        diskLight = disk->addComponent<Light>(LIGHT_DISK_SINGLE_FACE);
        diskLight->getParentTransform()->translate({0, 70, 0})->roll(glm::radians(180.f));
        diskLight->needUpdate();

        //==========
        auto quad = loadModel(quadModelPath);
        materialDatablock =
            (*quad->childBegin())->getComponent<MeshRenderer>()->getPrimitiveRenderer(0)->getMaterialDataBlock();
        memcpy(materialDatablock->getUBOData()->emission, emission, sizeof(float) * 3);
        memset(materialDatablock->getUBOData()->diffuse, 0, sizeof(float) * 3);
        quadLight = quad->addComponent<Light>(LIGHT_QUAD_SINGLE_FACE);
        quadLight->getParentTransform()->translate({0, 50, 0})->roll(glm::radians(180.f))->scale({20, 1, 20});
        quadLight->needUpdate();

        curLight = sphereLight;

        _editorCamera->getParentTransform()->setLocalTranslation({0, 30, 100});
    }
    void preparePipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };
        auto pipelineLayoutCI =
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};

        pipelineInfo.pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        pipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, fragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : pipelineInfo.shaders) {
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

        pipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            pipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void prepareShowLightPipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
        };
        auto pipelineLayoutCI =
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};

        showLightPipelineInfo.pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        showLightPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, showLightFragShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : showLightPipelineInfo.shaders) {
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

        showLightPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            showLightPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }

    void prepare() override {
        loadAssets();
        preparePipeline();
        prepareShowLightPipeline();
    }

    void drawGameObject(Shit::CommandBuffer *cmdBuffer, GameObject *gameObject,
                        Shit::PipelineLayout const *pipelineLayout) {
        if (!gameObject->isEnable()) return;
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

        // bind camera
        auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipelineInfo.pipelineLayout, 1, 1, &descriptorSet});

        // bind light
        descriptorSet = curLight->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipelineInfo.pipelineLayout, 3, 1, &descriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipeline});

        drawGameObject(cmdBuffer, modelObject, pipelineInfo.pipelineLayout);

        //===========================
        // draw light
        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, showLightPipelineInfo.pipeline});
        drawGameObject(cmdBuffer, curLight->getParent(), pipelineInfo.pipelineLayout);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setDefaultRenderPassClearValue() override {
        Shit::ClearColorValueFloat clearColor = {0.0, 0.0, 0.0, 1};
        Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
        _defaultRenderTarget.clearValues = {clearColor, clearDepthStencil};
        if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
            _defaultRenderTarget.clearValues.emplace_back(clearColor);
        }
    }
    void setupImGui() {
        ImGui::Begin("aaa");
        const char *items[]{
            "sphere",
            "tube",
            "disk",
            "quad",
        };
        static int index = 0;
        static bool flag = false;

        ImGuiComboFlags flags = 0;
        if (ImGui::BeginCombo("light type", items[index], flags)) {
            for (int i = 0; i < std::size(items); ++i) {
                const bool is_selected = (index == i);
                if (ImGui::Selectable(items[i], is_selected)) {
                    index = i;
                    curLight->getParent()->disable();
                    switch (index) {
                        case 0:
                            curLight = sphereLight;
                            break;
                        case 1:
                            curLight = tubeLight;
                            break;
                        case 2:
                            curLight = diskLight;
                            break;
                        case 3:
                            curLight = quadLight;
                            break;
                    }
                    curLight->getParent()->enable();
                    _device->WaitIdle();
                    flag = true;
                }
            }
            ImGui::EndCombo();
        }

        static Transform *lightTransform;
        static LightSSBO *lightSSBO;
        static glm::vec3 translation;
        static glm::vec3 rotation;
        lightTransform = curLight->getParentTransform();
        lightSSBO = curLight->getSSBOData();
        translation = lightTransform->getLocalTranslation();
        rotation = glm::degrees(glm::eulerAngles(lightTransform->getLocalRotation()));

        if (ImGui::DragFloat3("position", (float *)&translation, 0.1f)) {
            lightTransform->setLocalTranslation(translation);
            flag = true;
        }
        if (ImGui::DragFloat3("rotation", (float *)&rotation, 1.f)) {
            lightTransform->setLocalRotation(glm::quat(glm::radians(rotation)));
            flag = true;
        }
        if (index == 3) {
            // quad
            static glm::vec3 scale;
            scale = lightTransform->getLocalScale();
            if (ImGui::DragFloat3("scale", (float *)&scale, 0.1f, 0.1f)) {
                lightTransform->setLocalScale(scale);
                flag = true;
            }
        } else {
            static float scale;
            scale = lightTransform->getLocalScale().x;
            if (ImGui::DragFloat("scale", &scale, 0.1f, 0.1f)) {
                lightTransform->setLocalScale(glm::vec3{scale});
                flag = true;
            }
        }
        //
        ImGui::Separator();
        flag |= ImGui::DragFloat("light radiance", &lightSSBO->radiance, 0.1f, 0.1f);
        if (flag) {
            curLight->needUpdate();
            flag = false;
        }

        ImGui::End();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)