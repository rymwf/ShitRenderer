#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "IBL/display.vert";
static auto fragShaderPath = "IBL/display.frag";

static auto eq2CubeCompShaderName = "common/equirectangular2cube.comp";

static auto prefilteredEnvCompShaderName = "common/prefilteredEnv2D.comp";

static auto skyboxVertShaderPath = "IBL/skybox.vert";
static auto skyboxFragShaderPath = "IBL/skybox.frag";

static const char *testModelPaths[]{SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj",
                                    SHIT_SOURCE_DIR "/assets/models/obj/gearbox.obj"};

static char const *imagePaths[]{
    SHIT_SOURCE_DIR "/assets/images/limpopo_golf_course_4k.hdr",
};

static auto envBRDFImagePath = SHIT_SOURCE_DIR "/assets/images/envBRDF.hdr";

#define SKYBOX_WIDTH 1024
#define SAMPLE_NUM 512
#define MAX_ROUGHNESS_LEVEL 7

struct PipelineInfo {
    std::vector<Shader *> shaders;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    Shit::Pipeline *pipeline;
};

struct UBOOther {
    glm::vec3 baseColor;
    float metallic;
    float roughness;
    int useManualBaseColor;
    int enableACESFilmic;
    int renderMode;
    int Roughness1Lod;
    int diffEnable{true};
    int specEnable{true};
    int multibounceEnable{true};
    int emissionEnable{true};
    float clearCoatThickness;
    float clearCoatRoughness;
};
struct SkyboxUBOOther {
    int enableACESFilmic;
};

class Hello : public AppBase {
    std::vector<GameObject *> modelObjects;
    GameObject *helmet;
    int sceneIndex = 0;

    Light *light;
    MaterialDataBlock *pMaterialDataBlock;
    std::vector<Image *> equirectangularImages;
    std::vector<Texture *> equirectangularTexs;
    Texture *cubeTex;
    Image *envBRDFImage;
    Texture *envBRDFTex;
    Shit::Sampler *envBRDFTexSampler;
    bool showSkybox = true;

    PipelineInfo prefilteredEnvPipelineInfo;
    UBOOther uboOtherData{};
    Shit::Buffer *uboOtherBuffer;

    SkyboxUBOOther skyboxUboOtherData{};
    Shit::Buffer *skyboxUboOtherBuffer{};
    PipelineInfo eq2cubePipelineInfo;
    PipelineInfo skyboxPipelineInfo;
    PipelineInfo pipelineInfo;

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPaths[0]));
        model->load();
        auto model2 = static_cast<Model *>(_modelManager->create(testModelPaths[1]));
        model2->load();

        // auto materialId =
        // (*model->meshBegin())->primitives[0]->faceMaterialIds[0]; if (materialId
        // == -1)
        //	pMaterialDataBlock =
        //_materialDataBlockManager->getMaterialDataBlockById();
        // else
        //	pMaterialDataBlock =
        //_materialDataBlockManager->getMaterialDataBlockById(materialId);

        // create game objects
        int column = MAX_ROUGHNESS_LEVEL + 1, interval = 2, i = 0, j = 0, row = 3;
        for (; j < row; ++j) {
            for (i = 0; i < column; ++i) {
                auto a = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
                a->getComponent<Transform>()->translate(
                    {float(2 * i - column + 1) / 2 * interval, float(2 * j - row + 1) / 2 * interval, 0});
                modelObjects.emplace_back(a);
                for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
                    auto child = a->addChild();
                    auto meshRenderer = child->addComponent<MeshRenderer>(it->get());
                    auto materialDataBlock = meshRenderer->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
                    materialDataBlock->getUBOData()->roughness = float(i) / (std::max)(float(column - 1), 0.01f);
                    materialDataBlock->getUBOData()->metallic = float(j) / (std::max)(float(row - 1), 0.01f);
                    materialDataBlock->update();
                }
            }
        }
        {
            helmet = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
            for (auto it = model2->meshBegin(), end = model2->meshEnd(); it != end; ++it) {
                helmet->addChild()->addComponent<MeshRenderer>(it->get());
            }
            helmet->disable();
        }

        _editorCamera->getParentTransform()->setLocalTranslation({0, 0, 10});

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));

        // load images
        for (auto e : imagePaths) {
            auto a = equirectangularImages.emplace_back(static_cast<Image *>(_imageManager->create(e)));
            auto b = equirectangularTexs.emplace_back(_textureManager->createTexture(a));
            b->prepare();
            b->getImage()->GenerateMipmap(Shit::Filter::LINEAR, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                          Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        }

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

        // other ubobuffer
        // ubo
        uboOtherData.Roughness1Lod = MAX_ROUGHNESS_LEVEL;
        uboOtherBuffer = _device->Create(
            Shit::BufferCreateInfo{
                {},
                sizeof(UBOOther),
                Shit::BufferUsageFlagBits::TRANSFER_DST_BIT | Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
                Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
            &uboOtherData);

        skyboxUboOtherBuffer = _device->Create(
            Shit::BufferCreateInfo{
                {},
                sizeof(SkyboxUBOOther),
                Shit::BufferUsageFlagBits::TRANSFER_DST_BIT | Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
                Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
            &skyboxUboOtherData);
    }
    void prepareEq2CubePipeline() {
        //==============
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            //
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        eq2cubePipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, eq2cubePipelineInfo.descriptorSets.data());

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        //==============
        eq2cubePipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, eq2CubeCompShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        eq2cubePipelineInfo.shaders[0]->load();

        // generate cubemap pipeline
        // uint32_t constantIDs0[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
        // uint32_t constantValues0[] = {SKYBOX_WIDTH}; // cubemap width

        Shit::PipelineShaderStageCreateInfo shaderStageCI{
            eq2cubePipelineInfo.shaders[0]->getStage(),
            eq2cubePipelineInfo.shaders[0]->getHandle(),
            eq2cubePipelineInfo.shaders[0]->getEntryName().data(),
        };

        eq2cubePipelineInfo.pipeline = _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, pipelineLayout});
    }
    void prepareSkyboxPipeline() {
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {2, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };

        Shit::DescriptorSetLayout *descriptorSetLayouts[]{
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings}),
            _cameraDescriptorSetLayout,
        };

        skyboxPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayouts[0]}, skyboxPipelineInfo.descriptorSets.data());

        auto pipelineLayout =
            _device->Create(Shit::PipelineLayoutCreateInfo{std::size(descriptorSetLayouts), descriptorSetLayouts});

        //==============
        skyboxPipelineInfo.shaders = {
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, skyboxVertShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, skyboxFragShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}})),
        };
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : skyboxPipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        skyboxPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            {},
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, Shit::CullMode::BACK, Shit::FrontFace::COUNTER_CLOCKWISE, false, 0,
             0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                _sampleCount,
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
            0});
    }
    void preparePrefilteredEnvPipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            //
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };
        Shit::PushConstantRange pushConstantRanges[]{
            {Shit::ShaderStageFlagBits::COMPUTE_BIT, 2, 0, sizeof(float) * 2},
        };

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});
        auto levels = cubeTex->getImage()->GetCreateInfoPtr()->mipLevels;
        std::vector<Shit::DescriptorSetLayout const *> setLayouts(levels, descriptorSetLayout);
        prefilteredEnvPipelineInfo.descriptorSets.resize(levels);
        _descriptorPool->Allocate({levels, setLayouts.data()}, prefilteredEnvPipelineInfo.descriptorSets.data());

        auto pipelineLayout = _device->Create(
            Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout, std::size(pushConstantRanges), pushConstantRanges});

        //==============
        prefilteredEnvPipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, prefilteredEnvCompShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        prefilteredEnvPipelineInfo.shaders[0]->load();
        Shit::PipelineShaderStageCreateInfo shaderStageCI{prefilteredEnvPipelineInfo.shaders[0]->getStage(),
                                                          prefilteredEnvPipelineInfo.shaders[0]->getHandle(),
                                                          prefilteredEnvPipelineInfo.shaders[0]->getEntryName().data()};

        prefilteredEnvPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, pipelineLayout});
    }
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {2, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            //{10, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
            // Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        pipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, pipelineInfo.descriptorSets.data());

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout, _cameraDescriptorSetLayout, _materialDescriptorSetLayout,
            _lightDescriptorSetLayout, descriptorSetLayout};

        auto pipelineLayout = _device->Create(
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

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
    Texture *createCubemp() {
        auto tex = _textureManager->createTexture(
            Shit::ImageCreateInfo{Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
                                  Shit::ImageType::TYPE_2D,
                                  Shit::Format::R32G32B32A32_SFLOAT,
                                  {SKYBOX_WIDTH, SKYBOX_WIDTH, 1},
                                  0,
                                  6,
                                  Shit::SampleCountFlagBits::BIT_1,
                                  Shit::ImageTiling::OPTIMAL,
                                  Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::STORAGE_BIT |
                                      Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            Shit::ImageViewType::TYPE_CUBE, 0);
        tex->prepare();
        return tex;
    }
    void updateCubeTex(Texture *eqTex, Texture *cubeTex_) {
        //============
        auto sampler = g_App->getSampler("linear");
        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, eqTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {0, cubeTex_->getImageView(), Shit::ImageLayout::GENERAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {eq2cubePipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
             &imageInfos[0]},
            {eq2cubePipelineInfo.descriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imageInfos[1]},
        };
        _device->UpdateDescriptorSets({writeSets, writeSets + std::size(writeSets)});
        // run pipeline
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            Shit::ImageMemoryBarrier barrier{
                Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                Shit::AccessFlagBits::SHADER_WRITE_BIT,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                Shit::ImageLayout::GENERAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                cubeTex->getImage(),
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, cubeTex->getImage()->GetCreateInfoPtr()->mipLevels, 0,
                 cubeTex->getImage()->GetCreateInfoPtr()->arrayLayers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                 Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                 {},
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 1,
                                                                 &barrier});

            auto pipeline = dynamic_cast<Shit::ComputePipeline *>(eq2cubePipelineInfo.pipeline);
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, eq2cubePipelineInfo.pipeline});

            cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::COMPUTE, pipeline->GetCreateInfoPtr()->pLayout, 0,
                                           (uint32_t)eq2cubePipelineInfo.descriptorSets.size(),
                                           eq2cubePipelineInfo.descriptorSets.data()});
            cmdBuffer->Dispatch({SKYBOX_WIDTH, 6, 1});
        });
    }
    void updatePrefilteredEnvTex(Texture *eqTex, Texture *cubeTex_) {
        auto pipeline = dynamic_cast<Shit::ComputePipeline *>(prefilteredEnvPipelineInfo.pipeline);
        auto descriptorSetLayout = pipeline->GetCreateInfoPtr()->pLayout->GetCreateInfoPtr()->pSetLayouts[0];
        auto levels = cubeTex_->getImage()->GetCreateInfoPtr()->mipLevels;
        std::vector<Shit::DescriptorSetLayout const *> setLayouts(levels, descriptorSetLayout);

        auto sampler = getSampler("trilinear");
        std::vector<Shit::DescriptorImageInfo> imageInfos{
            {sampler, eqTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, {}};

        std::vector<Shit::ImageView *> cubeTexImageViews(levels, 0);
        for (uint32_t i = 0; i < levels; ++i) {
            cubeTexImageViews[i] =
                _device->Create(Shit::ImageViewCreateInfo{cubeTex_->getImage(),
                                                          Shit::ImageViewType::TYPE_CUBE,
                                                          cubeTex_->getImage()->GetCreateInfoPtr()->format,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0,
                                                           cubeTex_->getImage()->GetCreateInfoPtr()->arrayLayers}});
            imageInfos[1] = {nullptr, cubeTexImageViews[i], Shit::ImageLayout::GENERAL};
            Shit::WriteDescriptorSet writeSets[]{
                {prefilteredEnvPipelineInfo.descriptorSets[i], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                 &imageInfos[0]},
                {prefilteredEnvPipelineInfo.descriptorSets[i], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE,
                 &imageInfos[1]},
            };
            _device->UpdateDescriptorSets(writeSets);
        }

        // run pipeline
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, prefilteredEnvPipelineInfo.pipeline});

            auto pipeline = dynamic_cast<Shit::ComputePipeline *>(prefilteredEnvPipelineInfo.pipeline);
            auto descriptorSetLayout = pipeline->GetCreateInfoPtr()->pLayout->GetCreateInfoPtr()->pSetLayouts[0];
            auto levels = cubeTex->getImage()->GetCreateInfoPtr()->mipLevels;
            uint32_t width = cubeTex->getImage()->GetCreateInfoPtr()->extent.width;
            int sampleNum = SAMPLE_NUM;
            float a[2];
            memcpy(&a[1], &sampleNum, sizeof(int));
            int i = 1;
            for (; i < MAX_ROUGHNESS_LEVEL + 1; ++i) {
                width /= 2;
                a[0] = float(i) / MAX_ROUGHNESS_LEVEL;
                cmdBuffer->PushConstants(Shit::PushConstantInfo{pipeline->GetCreateInfoPtr()->pLayout,
                                                                Shit::ShaderStageFlagBits::COMPUTE_BIT, 0,
                                                                sizeof(float) * 2, a});
                cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                    Shit::PipelineBindPoint::COMPUTE, pipeline->GetCreateInfoPtr()->pLayout, 0, 1,
                    &prefilteredEnvPipelineInfo.descriptorSets[i]});
                cmdBuffer->Dispatch({width, 6, 1});
            }
            // transfer imagelayout to shder readonly
            Shit::ImageMemoryBarrier barrier{
                Shit::AccessFlagBits::SHADER_WRITE_BIT,
                Shit::AccessFlagBits::SHADER_READ_BIT,
                Shit::ImageLayout::GENERAL,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                cmdBuffer->GetCommandPool()->GetCreateInfoPtr()->queueFamilyIndex,
                _graphicsQueue->GetFamilyIndex(),
                cubeTex->getImage(),
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, cubeTex->getImage()->GetCreateInfoPtr()->mipLevels, 0,
                 cubeTex->getImage()->GetCreateInfoPtr()->arrayLayers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                 Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                                 {},
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 1,
                                                                 &barrier});
        });
        // takeScreenshot(_device, cubeTex->getImage(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }

    void writeSkyboDescriptorSet() {
        Shit::DescriptorBufferInfo bufferInfo{skyboxUboOtherBuffer, 0, sizeof(SkyboxUBOOther)};
        Shit::DescriptorImageInfo imageInfo{getSampler("linear"), cubeTex->getImageView(),
                                            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
        Shit::WriteDescriptorSet writes[]{
            {skyboxPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfo},
            {skyboxPipelineInfo.descriptorSets[0], 2, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo}};
        _device->UpdateDescriptorSets(writes);
    }
    void writeDisplayDescriptorSet() {
        auto sampler = getSampler("trilinear");

        // write
        Shit::DescriptorBufferInfo bufferInfo{uboOtherBuffer, 0, sizeof(UBOOther)};
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, cubeTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {envBRDFTexSampler, envBRDFTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {pipelineInfo.descriptorSets[0], 2, 0, 1, Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo},
            {pipelineInfo.descriptorSets[0], 8, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
            {pipelineInfo.descriptorSets[0], 9, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[1]},
        };
        _device->UpdateDescriptorSets(writeSets);
    }
    void updateUBOOtherData() {
        void *data;
        uboOtherBuffer->MapMemory(0, sizeof(UBOOther), &data);
        memcpy(data, &uboOtherData, sizeof(UBOOther));
        uboOtherBuffer->UnMapMemory();
    }
    void updateSkyboUBOOtherData() {
        void *data;
        skyboxUboOtherBuffer->MapMemory(0, sizeof(SkyboxUBOOther), &data);
        memcpy(data, &skyboxUboOtherData, sizeof(SkyboxUBOOther));
        skyboxUboOtherBuffer->UnMapMemory();
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

        cmdBuffer->SetViewport({0, 1, &viewport});
        cmdBuffer->SetScissor({0, 1, &scissor});

        // bind camera
        auto tempDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(skyboxPipelineInfo.pipeline);
        if (showSkybox) {
            cmdBuffer->BindDescriptorSets(
                {Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &tempDescriptorSet});
            cmdBuffer->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, skyboxPipelineInfo.pipeline});
            cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 0,
                                           (uint32_t)skyboxPipelineInfo.descriptorSets.size(),
                                           skyboxPipelineInfo.descriptorSets.data()});
            cmdBuffer->Draw(Shit::DrawIndirectCommand{36, 1, 0, 0});
        }

        //======================
        pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo.pipeline);
        cmdBuffer->BindDescriptorSets(
            {Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &tempDescriptorSet});

        // bind light
        tempDescriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(
            {Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &tempDescriptorSet});

        // bind cube
        cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 4, 1,
                                       &pipelineInfo.descriptorSets[0]});

        cmdBuffer->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        static bool flag = false;
        static bool updateSkyboxflag = false;

        ImGui::Begin("tools");
        // show skybox
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("show skybox", &showSkybox);

        _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("spheres", &sceneIndex, 0);
        _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("helmet", &sceneIndex, 1);

        updateSkyboxflag |= flag |= ImGui::Checkbox("ACESFilmic", (bool *)&uboOtherData.enableACESFilmic);
        skyboxUboOtherData.enableACESFilmic = uboOtherData.enableACESFilmic;

        if (sceneIndex == 0) {
            for (auto e : modelObjects) e->enable();
            helmet->disable();
        } else if (sceneIndex = 1) {
            for (auto e : modelObjects) e->disable();
            helmet->enable();
        }
        // camera pos
        {
            ImGui::Text("camera edit");
            static float dist = 10.;
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

        static int imageIndex = 0;
        auto eqImageName = std::filesystem::path(imagePaths[imageIndex]).filename().string();
        if (ImGui::BeginCombo("skybox image", eqImageName.data())) {
            for (int i = 0; i < std::size(imagePaths); ++i) {
                const bool is_selected = imageIndex == i;
                if (ImGui::Selectable(std::filesystem::path(imagePaths[i]).filename().string().data(), is_selected)) {
                    imageIndex = i;
                    updateCubeTex(equirectangularTexs[i], cubeTex);
                    updatePrefilteredEnvTex(equirectangularTexs[i], cubeTex);
                }
            }
            ImGui::EndCombo();
        }

        flag |= ImGui::Checkbox("diffEnable", (bool *)&uboOtherData.diffEnable);
        flag |= ImGui::Checkbox("specEnable", (bool *)&uboOtherData.specEnable);
        flag |= ImGui::Checkbox("multibounceEnable", (bool *)&uboOtherData.multibounceEnable);
        flag |= ImGui::Checkbox("emissionEnable", (bool *)&uboOtherData.emissionEnable);

        flag |= ImGui::Checkbox("use manual basecolor", (bool *)&uboOtherData.useManualBaseColor);
        if (uboOtherData.useManualBaseColor) {
            flag |= ImGui::SliderFloat("manual metallic", &uboOtherData.metallic, 0, 1);
            flag |= ImGui::SliderFloat("manual roughness", &uboOtherData.roughness, 0, 1);

            static std::vector<std::pair<std::string, glm::vec3>> baseColors{
                {"Titanium ", {0.542, 0.497, 0.449}},    {"Chromium ", {0.549, 0.556, 0.554}},
                {"Iron ", {0.562, 0.565, 0.578}},        {"Nickel ", {0.660, 0.609, 0.526}},
                {"Platinum ", {0.673, 0.637, 0.585}},    {"Copper ", {0.955, 0.638, 0.538}},
                {"Palladium ", {0.733, 0.697, 0.652}},   {"Mercury ", {0.781, 0.780, 0.778}},
                {"Brass C260) ", {0.910, 0.778, 0.423}}, {"Zinc ", {0.664, 0.824, 0.850}},
                {"Gold ", {1.000, 0.782, 0.344}},        {"Gold2 ", {0.604, 0.440, 0.012}},
                {"Aluminum ", {0.913, 0.922, 0.924}},    {"Silver", {0.972, 0.960, 0.915}},
            };
            for (int i = 0; i < baseColors.size(); ++i) {
                if (i % 3) ImGui::SameLine();
                if (ImGui::Button(baseColors[i].first.data())) {
                    uboOtherData.baseColor = baseColors[i].second;
                    flag = true;
                }
            }
            ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreviewHalf |
                                             ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;
            flag |= ImGui::ColorEdit3("baseColor", (float *)&uboOtherData.baseColor,
                                      ImGuiColorEditFlags_NoLabel | misc_flags);

            flag |= ImGui::SliderFloat("clearCoatThickness", &uboOtherData.clearCoatThickness, 0, 1);
            flag |= ImGui::SliderFloat("clearCoatRoughness", &uboOtherData.clearCoatRoughness, 0, 1);
        }

        if (flag) {
            updateUBOOtherData();
            flag = false;
        }
        if (updateSkyboxflag) {
            updateSkyboUBOOtherData();
            updateSkyboxflag = false;
        }
        ImGui::End();
    }
    void prepare() override {
        loadAssets();
        cubeTex = createCubemp();

        prepareEq2CubePipeline();
        preparePrefilteredEnvPipeline();
        prepareSkyboxPipeline();
        preparePipeline();

        auto eqTex = equirectangularTexs[0];
        updateCubeTex(eqTex, cubeTex);
        updatePrefilteredEnvTex(eqTex, cubeTex);

        writeSkyboDescriptorSet();
        writeDisplayDescriptorSet();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)