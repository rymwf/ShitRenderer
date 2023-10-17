#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "sheencloth/display.vert";
static auto fragShaderPath = "sheencloth/display.frag";

static const char *testModelPaths[]{SHIT_SOURCE_DIR "/assets/models/obj/SheenCloth/SheenCloth.obj",
                                    SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj"};

static auto envBRDFImagePath = SHIT_SOURCE_DIR "/assets/images/envBRDF.hdr";
static auto sheenEnvBRDFImagePath = SHIT_SOURCE_DIR "/assets/images/sheenEnvBRDF.hdr";

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
    int renderMode{3};  // bit 0: base layer ,1 sheen
    int enableAces;
    float sheenRoughnessFactor{1};
    float padding0;
    float ambientRadiance[3];
    float padding1;
    float sheenColor[4]{1, 1, 1, 0.5};
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;

    std::vector<GameObject *> modelObjects;

    Light *light;
    PCParameter uPC{};

    MaterialDataBlock *pMaterialDataBlock;

    Image *envBRDFImage;
    Texture *envBRDFTex;

    Image *sheenEnvBRDFImage;
    Texture *sheenEnvBRDFTex;

    Shit::Sampler *envBRDFTexSampler;

    int sceneIndex{0};

public:
    void loadAssets() {
        std::vector<Model *> models;
        for (auto e : testModelPaths) {
            models.emplace_back(static_cast<Model *>(_modelManager->create(e)))->load();
            modelObjects.emplace_back(_gameObjects.emplace_back(std::make_unique<GameObject>()).get());
        }
        for (auto it = models[0]->meshBegin(), end = models[0]->meshEnd(); it != end; ++it) {
            modelObjects[0]->addChild()->addComponent<MeshRenderer>(it->get());
        }
        for (auto it = models[1]->meshBegin(), end = models[1]->meshEnd(); it != end; ++it) {
            auto child = modelObjects[1]->addChild();
            auto a = child->addComponent<MeshRenderer>(it->get());
            pMaterialDataBlock = a->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
        }
        modelObjects[0]->enable();
        modelObjects[1]->disable();

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));

        _editorCamera->getParentTransform()->setLocalTranslation({0, 1.8, 12});
        _editorCamera->setFocalLength(0.05);

        envBRDFImage = static_cast<Image *>(_imageManager->create(envBRDFImagePath));
        envBRDFTex = _textureManager->createOrRetrieveTexture(envBRDFImage);
        envBRDFTex->prepare();

        sheenEnvBRDFImage = static_cast<Image *>(_imageManager->create(sheenEnvBRDFImagePath));
        sheenEnvBRDFTex = _textureManager->createOrRetrieveTexture(sheenEnvBRDFImage);
        sheenEnvBRDFTex->prepare();

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
        Shit::DescriptorImageInfo imageInfos[]{
            {envBRDFTexSampler, envBRDFTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {envBRDFTexSampler, sheenEnvBRDFTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}};

        Shit::WriteDescriptorSet writes[] = {
            {pipelineInfo.descriptorSets[0], 8, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
            {pipelineInfo.descriptorSets[0], 9, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[1]},
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
        auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &descriptorSet});

        // bind light
        descriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &descriptorSet});

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
        _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("cloth", &sceneIndex, 0);
        _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("sphere", &sceneIndex, 1);

        if (_needUpdateDefaultCommandBuffer) {
            switch (sceneIndex) {
                case 0:
                default: {
                    modelObjects[0]->enable();
                    modelObjects[1]->disable();
                } break;
                case 1:
                    modelObjects[0]->disable();
                    modelObjects[1]->enable();
                    break;
            }
        }

        ImGui::Separator();
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enableAces", (bool *)&uPC.enableAces);
        {
            ImGui::Text("light edit");
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

        static bool enableBaseLayer = uPC.renderMode & 1, enableSheenLayer = uPC.renderMode & 2;
        static bool flag = false;
        flag |= ImGui::Checkbox("baseLayer", &enableBaseLayer);
        flag |= ImGui::Checkbox("sheenLayer", &enableSheenLayer);
        if (flag) {
            if (enableBaseLayer)
                uPC.renderMode |= 1;
            else
                uPC.renderMode &= ~1;

            if (enableSheenLayer)
                uPC.renderMode |= 2;
            else
                uPC.renderMode &= ~2;
            _needUpdateDefaultCommandBuffer = true;
            flag = false;
        }
        _needUpdateDefaultCommandBuffer |=
            ImGui::ColorEdit4("sheenColor", uPC.sheenColor, ImGuiColorEditFlags_NoLabel | misc_flags);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("sheenRoughnessFactor", &uPC.sheenRoughnessFactor, 0, 1);

        //===============================
        // ImGui::Separator();
        auto materialData = pMaterialDataBlock->getUBOData();

        static bool materialFlag = false;
        materialFlag |= ImGui::ColorEdit3("diffuse", materialData->diffuse, ImGuiColorEditFlags_NoLabel | misc_flags);
        materialFlag |= ImGui::SliderFloat("metallic", &materialData->metallic, 0, 1);
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