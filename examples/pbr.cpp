#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "pbr/display.vert";
static auto fragShaderPath = "pbr/display.frag";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj"; static auto
// testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/bunny.obj";
static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj";

static auto envBRDFImagePath = SHIT_SOURCE_DIR "/assets/images/envBRDF.hdr";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    Shit::Pipeline *pipeline;
    // Shit::PipelineLayoutCreateInfo pipelineLayoutCI;
    // Shit::PipelineLayout *pipelineLayout;
};

struct PCParameter {
    int diffModel{1};
    int specModel{1};
    int multibounceModel{1};
    int enableAces{};
    float ambientRadiance[3]{};
    int ambientDiffModel{1};
    int ambientSpecModel{1};
    int ambientMultibounceModel{1};
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;
    GameObject *modelObject;
    Light *light;
    MaterialDataBlock *pMaterialDataBlock;

    Image *envBRDFImage;
    Texture *envBRDFTex;
    Shit::Sampler *envBRDFTexSampler;

    PCParameter uPC{};

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        // modelObject->getComponent<Transform>()
        //	->pitch(glm::radians(-90.f))
        //	->roll(glm::radians(-90.f));

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto a = modelObject->addChild()->addComponent<MeshRenderer>(it->get());
            pMaterialDataBlock = a->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
        }
        modelObject->prepare();

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        // light->getParentTransform()->pitch(glm::radians(-90.f));

        // envbrdf
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

        _defaultRenderTarget.clearValues = {Shit::ClearColorValueFloat{0.1, 0.2, 0.1, 1},
                                            Shit::ClearDepthStencilValue{1, 0},
                                            Shit::ClearColorValueFloat{0.0, 0.0, 0.0, 0}};
        uPC.ambientRadiance[0] = std::get<0>(_defaultRenderTarget.clearValues[0])[0];
        uPC.ambientRadiance[1] = std::get<0>(_defaultRenderTarget.clearValues[0])[1];
        uPC.ambientRadiance[2] = std::get<0>(_defaultRenderTarget.clearValues[0])[2];
    }
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
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

        auto pipelineLayoutCI = Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                               descriptorSetLayouts.data(), 1, &pcRange};

        auto pipelineLayout = _device->Create(pipelineLayoutCI);

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
            pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void writeDescriptorSets() {
        auto sampler = getSampler("linear");

        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {envBRDFTexSampler, envBRDFTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {pipelineInfo.descriptorSets[0], 8, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
        };
        _device->UpdateDescriptorSets(writeSets);
    }
    void prepare() override {
        loadAssets();
        preparePipeline();
        writeDescriptorSets();
        _editorCamera->getParentTransform()->setDefaultLocalTranslation({0, 0, 2.5});
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
        ImGui::Begin("editor");
        // camera pos
        {
            ImGui::Text("camera edit");
            static float dist = 2.5;
            ImGui::InputFloat("dist", &dist);

            static bool orth = false;
            if (ImGui::Checkbox("orthogonal", &orth)) {
                static auto a = *_editorCamera->getPerspectiveDesc();
                if (orth) {
                    auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
                    _editorCamera->setOrthogonal(dist * float(ext.width) / ext.height, dist);
                } else {
                    _editorCamera->setPerspective(a.fovy, a.aspect);
                }
            }

            auto &&cameraPos = _editorCamera->getParentTransform()->getLocalTranslation();
            if (ImGui::InputFloat3("cameraPos", (float *)&cameraPos)) {
                _editorCamera->getParentTransform()->setLocalTranslation(cameraPos);
            }
            if (ImGui::Button("+X")) {
                _editorCamera->getParentTransform()
                    ->reset()
                    ->setLocalTranslation(glm::vec3(0, 0, dist))
                    ->rotate(glm::vec3(0, glm::radians(90.f), 0));
            }
            ImGui::SameLine();
            if (ImGui::Button("-X")) {
                _editorCamera->getParentTransform()
                    ->reset()
                    ->setLocalTranslation(glm::vec3(0, 0, dist))
                    ->rotate(glm::vec3(0, glm::radians(-90.f), 0));
            }
            ImGui::SameLine();
            if (ImGui::Button("+Z")) {
                _editorCamera->getParentTransform()->reset()->setLocalTranslation(glm::vec3(0, 0, dist));
            }
            ImGui::SameLine();
            if (ImGui::Button("-Z")) {
                _editorCamera->getParentTransform()
                    ->reset()
                    ->setLocalTranslation(glm::vec3(0, 0, dist))
                    ->rotate(glm::vec3(0, glm::radians(180.f), 0));
            }
        }
        ImGui::Separator();
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
        ImGui::Separator();
        {
            ImGui::Text("material edit");
            static bool flag = false;

            _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enableAces", (bool *)&uPC.enableAces);

            static const char *diffuseModels[]{
                "none", "lambertian", "lambertianPlus", "Shirley", "AshikhminShirley",
            };
            static const char *specularModels[]{
                "none", "BlinnPhong", "Phong", "BRDFBeckmann", "BRDFBlinnPhong", "BRDFGGX",
            };
            static const char *multiBounceModels[]{
                "none",
                "multibounce",
            };
            static const char *ambientDiffModels[]{
                "none",
                "diff",
            };
            static const char *ambientSpecModels[]{
                "none",
                "spec",
            };
            static const char *ambientSpecMsModels[]{
                "none",
                "multibounce",
            };

            const char *diffModel = diffuseModels[uPC.diffModel];
            const char *specModel = specularModels[uPC.specModel];
            const char *msModel = multiBounceModels[uPC.multibounceModel];
            const char *ambientDiffModel = ambientDiffModels[uPC.ambientDiffModel];
            const char *ambientSpecModel = ambientSpecModels[uPC.ambientSpecModel];
            const char *ambientSpecMsModel = ambientSpecMsModels[uPC.ambientMultibounceModel];

            if (ImGui::BeginCombo("diff", diffModel)) {
                for (int i = 0; i < std::size(diffuseModels); ++i) {
                    const bool is_selected = (uPC.diffModel == i);
                    if (ImGui::Selectable(diffuseModels[i], is_selected)) {
                        uPC.diffModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("spec", specModel)) {
                for (int i = 0; i < std::size(specularModels); ++i) {
                    const bool is_selected = (uPC.specModel == i);
                    if (ImGui::Selectable(specularModels[i], is_selected)) {
                        uPC.specModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("multibounce", msModel)) {
                for (int i = 0; i < std::size(multiBounceModels); ++i) {
                    const bool is_selected = (uPC.multibounceModel == i);
                    if (ImGui::Selectable(multiBounceModels[i], is_selected)) {
                        uPC.multibounceModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
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
            if (ImGui::BeginCombo("ambientDiffModel", ambientDiffModel)) {
                for (int i = 0; i < std::size(ambientDiffModels); ++i) {
                    const bool is_selected = (uPC.ambientDiffModel == i);
                    if (ImGui::Selectable(ambientDiffModels[i], is_selected)) {
                        uPC.ambientDiffModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("ambientSpecModel", ambientSpecModel)) {
                for (int i = 0; i < std::size(ambientSpecModels); ++i) {
                    const bool is_selected = (uPC.ambientSpecModel == i);
                    if (ImGui::Selectable(ambientSpecModels[i], is_selected)) {
                        uPC.ambientSpecModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::BeginCombo("ambientMultibounceModel", ambientSpecMsModel)) {
                for (int i = 0; i < std::size(ambientSpecMsModels); ++i) {
                    const bool is_selected = (uPC.ambientMultibounceModel == i);
                    if (ImGui::Selectable(ambientSpecMsModels[i], is_selected)) {
                        uPC.ambientMultibounceModel = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            auto materialData = pMaterialDataBlock->getUBOData();

            // linear
            static std::vector<std::pair<std::string, glm::vec3>> materials{
                {"Titanium ", {0.542, 0.497, 0.449}},    {"Chromium ", {0.549, 0.556, 0.554}},
                {"Iron ", {0.562, 0.565, 0.578}},        {"Nickel ", {0.660, 0.609, 0.526}},
                {"Platinum ", {0.673, 0.637, 0.585}},    {"Copper ", {0.955, 0.638, 0.538}},
                {"Palladium ", {0.733, 0.697, 0.652}},   {"Mercury ", {0.781, 0.780, 0.778}},
                {"Brass C260) ", {0.910, 0.778, 0.423}}, {"Zinc ", {0.664, 0.824, 0.850}},
                {"Gold ", {1.000, 0.782, 0.344}},        {"Gold2 ", {0.604, 0.440, 0.012}},
                {"Aluminum ", {0.913, 0.922, 0.924}},    {"Silver", {0.972, 0.960, 0.915}},
            };
            for (int i = 0; i < materials.size(); ++i) {
                if (i % 3) ImGui::SameLine();
                if (ImGui::Button(materials[i].first.data())) {
                    materialData->diffuse[0] = materials[i].second[0];
                    materialData->diffuse[1] = materials[i].second[1];
                    materialData->diffuse[2] = materials[i].second[2];
                    flag = true;
                }
            }

            flag |= ImGui::ColorEdit3("diffuse", materialData->diffuse, ImGuiColorEditFlags_NoLabel | misc_flags);
            flag |= ImGui::InputFloat("shininess", &materialData->shininess, 1);
            flag |= ImGui::SliderFloat("metallic", &materialData->metallic, 0, 1);
            flag |= ImGui::SliderFloat("roughness", &materialData->roughness, 0, 1);
            flag |= ImGui::SliderFloat("sheen", &materialData->sheen, 0, 1);
            flag |= ImGui::SliderFloat("clearcoat_thickness", &materialData->clearcoat_thickness, 0, 1);
            flag |= ImGui::SliderFloat("clearcoat_roughness", &materialData->clearcoat_roughness, 0, 1);
            flag |= ImGui::SliderFloat("anisotropy", &materialData->anisotropy, 0, 1);
            flag |= ImGui::InputFloat("anisotropy_rotation", &materialData->anisotropy_rotation);
            if (flag) {
                pMaterialDataBlock->update();
                flag = false;
            }
        }
        ImGui::End();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)