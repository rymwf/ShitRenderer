#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "SSLarge/display.vert";
static auto fragShaderPath = "SSLarge/display.frag";

static const char *testModelPaths[]{SHIT_SOURCE_DIR "/assets/models/obj/hebe/hebe.obj"};

static auto envBRDFImagePath = SHIT_SOURCE_DIR "/assets/images/envBRDF.hdr";

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

struct PCParameter {
    float ambientRadiance[3];
    int renderMode{0};  // bit 0: base layer
    int enableAces;
    float thicknessOffset{0.0};
    float thicknessScale{3};
    float fLTDistortion{0.2};
    float fLTScale{0.1};
    float fLTPower{0.5};
    float kwrap{0.5};
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
    PipelineInfo pipelineInfo;

    std::vector<GameObject *> modelObjects;

    Light *light;
    PCParameter uPC{};

    MaterialDataBlock *pMaterialDataBlock;

    ModelController *modelController;

    Image *envBRDFImage;
    Texture *envBRDFTex;

    Shit::Sampler *envBRDFTexSampler;

public:
    void loadAssets() {
        std::vector<Model *> models;
        for (auto e : testModelPaths) {
            models.emplace_back(static_cast<Model *>(_modelManager->create(e)))->load();
            modelObjects.emplace_back(_gameObjects.emplace_back(std::make_unique<GameObject>()).get());
        }
        for (auto it = models[0]->meshBegin(), end = models[0]->meshEnd(); it != end; ++it) {
            auto a = modelObjects[0]->addChild()->addComponent<MeshRenderer>(it->get());
            pMaterialDataBlock = a->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
        }
        pMaterialDataBlock->getUBOData()->diffuse[0] = 0;
        pMaterialDataBlock->getUBOData()->diffuse[1] = 0.78;
        pMaterialDataBlock->getUBOData()->diffuse[2] = 0;

        modelObjects[0]->enable();
        modelController = modelObjects[0]->addComponent<ModelController>();
        modelController->pause();

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        // light ->getParentTransform()->pitch(glm::radians(-60.f));
        _editorCamera->getParentTransform()->setLocalTranslation({0, 0, 12});
        _editorCamera->setFocalLength(0.05);
        _editorCamera->setFarPlane(20);

        envBRDFImage = static_cast<Image *>(_imageManager->create(envBRDFImagePath));
        envBRDFTex = _textureManager->createOrRetrieveTexture(envBRDFImage);
        envBRDFTex->prepare();

        envBRDFTexSampler =
            createSampler("envBRDFTexSampler", Shit::SamplerCreateInfo{Shit::Filter::LINEAR,
                                                                       Shit::Filter::LINEAR,
                                                                       Shit::SamplerMipmapMode::NEAREST,
                                                                       Shit::SamplerWrapMode::CLAMP_TO_EDGE,
                                                                       Shit::SamplerWrapMode::CLAMP_TO_EDGE,
                                                                       Shit::SamplerWrapMode::CLAMP_TO_EDGE,
                                                                       0,
                                                                       false,
                                                                       0,
                                                                       false,
                                                                       {},
                                                                       0,
                                                                       0,
                                                                       Shit::BorderColor::FLOAT_OPAQUE_BLACK});

        uPC.ambientRadiance[0] = std::get<0>(_defaultRenderTarget.clearValues[0])[0];
        uPC.ambientRadiance[1] = std::get<0>(_defaultRenderTarget.clearValues[0])[1];
        uPC.ambientRadiance[2] = std::get<0>(_defaultRenderTarget.clearValues[0])[2];
    }
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            //
            {8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        pipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, pipelineInfo.descriptorSets.data());

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout, _cameraDescriptorSetLayout, _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,     descriptorSetLayout,
        };
        Shit::PushConstantRange pcRange{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 2, 0, sizeof(PCParameter)};

        pipelineInfo.pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{
            (uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), 1u, &pcRange});

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
    void updateDescriptorSets() {
        auto sampler = getSampler("linear");

        Shit::DescriptorImageInfo imageInfos[]{
            {envBRDFTexSampler, envBRDFTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };

        Shit::WriteDescriptorSet writes[] = {
            {pipelineInfo.descriptorSets[0], 8, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
        };
        _device->UpdateDescriptorSets(writes);
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
        auto cameraDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        auto lightDescriptorSet = light->getDescriptorSet();

        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});
        auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo.pipeline);
        // bind camera
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &cameraDescriptorSet});

        // bind light
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &lightDescriptorSet});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline->GetCreateInfoPtr()->pLayout, 4, 1,
                                                                   &pipelineInfo.descriptorSets[0]});

        cmdBuffer->PushConstants({pipeline->GetCreateInfoPtr()->pLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0u,
                                  sizeof(PCParameter), &uPC});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("settings");
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enableAces", (bool *)&uPC.enableAces);

        static float rotateSpeed = modelController->getSpeed();
        if (ImGui::DragFloat("rotate speed", &rotateSpeed)) {
            modelController->setSpeed(rotateSpeed);
        }
        static bool pause = !modelController->isRunning();
        if (ImGui::Checkbox("pause", &pause)) {
            if (pause)
                modelController->pause();
            else
                modelController->run();
        }

        {
            ImGui::Text("light settings");

            auto uboLightData = light->getSSBOData();
            static bool flag = false;

            ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreviewHalf |
                                             ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;
            flag |= ImGui::ColorEdit3("lightcolor", (float *)&uboLightData->color,
                                      ImGuiColorEditFlags_NoLabel | misc_flags);
            // flag |= ImGui::InputFloat3("lightDirection", (float
            // *)&uboLightData->direction);
            flag |= ImGui::InputFloat("lightRadiance", (float *)&uboLightData->radiance);
            if (flag) {
                light->needUpdate();
                // light->update();
                flag = false;
            }
        }

        ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreviewHalf |
                                         ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;

        if (ImGui::ColorEdit3("ambientColor", uPC.ambientRadiance, ImGuiColorEditFlags_NoLabel | misc_flags)) {
            _needUpdateDefaultCommandBuffer = true;
            _defaultRenderTarget.clearValues[0] = {Shit::ClearColorValueFloat{
                uPC.ambientRadiance[0],
                uPC.ambientRadiance[1],
                uPC.ambientRadiance[2],
                1,
            }};
            if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
                _defaultRenderTarget.clearValues[2] = _defaultRenderTarget.clearValues[0];
            }
        }
        //=================
        ImGui::Separator();
        auto materialData = pMaterialDataBlock->getUBOData();

        static bool materialFlag = false;
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("thicknessOffset", &uPC.thicknessOffset, 0, 5);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("thicknessScale", &uPC.thicknessScale, 0, 20);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("fLTDistortion", &uPC.fLTDistortion, 0, 1);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("fLTPower", &uPC.fLTPower, 0, 1);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("fLTScale", &uPC.fLTScale, 0, 1);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("kwrap", &uPC.kwrap, 0, 1);

        materialFlag |= ImGui::ColorEdit3("diffuse", materialData->diffuse, ImGuiColorEditFlags_NoLabel | misc_flags);
        materialFlag |= ImGui::SliderFloat("roughness", &materialData->roughness, 0, 1);

        if (materialFlag) {
            pMaterialDataBlock->update();
            materialFlag = false;
        }
        ImGui::End();
    }
    void prepare() override {
        loadAssets();
        preparePipeline();
        updateDescriptorSets();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_1)