#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "irradianceSH/display.vert";
static auto fragShaderPath = "irradianceSH/display.frag";

static auto eq2CubeCompShaderName = "common/equirectangular2cube.comp";

static auto irradianceSHShaderName = "common/irradianceSH.comp";
static auto irradianceSHLlmShaderName = "common/irradianceSHLlm.comp";

static auto skyboxVertShaderPath = "skybox/skybox.vert";
static auto skyboxFragShaderPath = "skybox/skybox.frag";

static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj";
static auto imagePath = SHIT_SOURCE_DIR "/assets/images/limpopo_golf_course_4k.hdr";

#define SKYBOX_WIDTH 1024

struct PipelineInfo {
    std::vector<Shader *> shaders;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    Shit::Pipeline *pipeline;
};

class Hello : public AppBase {
    std::vector<GameObject *> modelObjects;
    Light *light;
    MaterialDataBlock *pMaterialDataBlock;
    Image *equirectangularImage;
    Texture *equirectangularTex;
    Texture *cubeTex;

    Texture *irradianceSHLlmTex;
    Texture *irradianceTex;

    PipelineInfo irradianceSHLlmPipelineInfo;
    PipelineInfo irradianceSHPipelineInfo;
    Shit::Buffer *uboBuffer;

    PipelineInfo eq2cubePipelineInfo;
    PipelineInfo skyboxPipelineInfo;
    PipelineInfo pipelineInfo;

public:
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        // auto materialId =
        // (*model->meshBegin())->primitives[0]->faceMaterialIds[0]; if (materialId
        // == -1)
        //	pMaterialDataBlock =
        //_materialDataBlockManager->getMaterialDataBlockById();
        // else
        //	pMaterialDataBlock =
        //_materialDataBlockManager->getMaterialDataBlockById(materialId);

        // create game objects
        int count = 1, interval = 2, i = 0;
        for (; i < count; ++i) {
            auto a = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
            a->getComponent<Transform>()->translate({float(2 * i - count + 1) / 2 * interval, 0, 0});
            modelObjects.emplace_back(a);
            for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
                auto child = a->addChild();
                auto meshRenderer = child->addComponent<MeshRenderer>(it->get());
                auto materialDataBlock = meshRenderer->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
                materialDataBlock->getUBOData()->roughness = i;
                materialDataBlock->update();
            }
        }
        _editorCamera->getParentTransform()->setLocalTranslation({0, 0, 10});

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));

        // load image
        equirectangularImage = static_cast<Image *>(_imageManager->create(imagePath));

        equirectangularTex = _textureManager->createTexture(equirectangularImage);
        // static const unsigned char pixel[]{unsigned char(1.0), unsigned char(0.2), unsigned char(0.2), unsigned
        // char(1)};

        // equirectangularTex = _textureManager->createTexture(
        //     Shit::ImageCreateInfo{{},
        //                           Shit::ImageType::TYPE_2D,
        //                           Shit::Format::R8G8B8A8_UNORM,
        //                           {1, 1, 1},
        //                           1,
        //                           1,
        //                           Shit::SampleCountFlagBits::BIT_1,
        //                           Shit::ImageTiling::OPTIMAL,
        //                           Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
        //                           Shit::MemoryPropertyFlagBits ::DEVICE_LOCAL_BIT,
        //                           Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        //     Shit::ImageViewType::TYPE_2D, pixel);
        equirectangularTex->prepare();
        equirectangularTex->getImage()->GenerateMipmap(Shit::Filter::LINEAR,
                                                       Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                       Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }
    void prepareSHLlmPipeline() {
        Shit::DescriptorSetLayoutBinding bindings[] = {
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT}};

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        irradianceSHLlmPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, irradianceSHLlmPipelineInfo.descriptorSets.data());

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        irradianceSHLlmPipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, irradianceSHLlmShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        auto p = irradianceSHLlmPipelineInfo.shaders[0];
        p->load();
        Shit::PipelineShaderStageCreateInfo shaderStageCI = {p->getStage(), p->getHandle(), p->getEntryName().data()};

        irradianceSHLlmPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, pipelineLayout});
    }
    void prepareSHPipeline() {
        Shit::DescriptorSetLayoutBinding bindings[] = {
            {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT}};

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        irradianceSHPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, irradianceSHPipelineInfo.descriptorSets.data());

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        irradianceSHPipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, irradianceSHShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        auto p = irradianceSHPipelineInfo.shaders[0];
        p->load();
        Shit::PipelineShaderStageCreateInfo shaderStageCI = {p->getStage(), p->getHandle(), p->getEntryName().data()};

        irradianceSHPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, pipelineLayout});
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
            eq2cubePipelineInfo.shaders[0]->getStage(), eq2cubePipelineInfo.shaders[0]->getHandle(),
            eq2cubePipelineInfo.shaders[0]->getEntryName().data(),
            //{1, constantIDs0, constantValues0}
        };

        eq2cubePipelineInfo.pipeline = _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, pipelineLayout});
    }
    void prepareSkyboxPipeline() {
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
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
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding binding = {9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                                    Shit::ShaderStageFlagBits::FRAGMENT_BIT};
        auto reflectDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &binding});

        pipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &reflectDescriptorSetLayout}, pipelineInfo.descriptorSets.data());

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout, _cameraDescriptorSetLayout, _materialDescriptorSetLayout,
            _lightDescriptorSetLayout, reflectDescriptorSetLayout};

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
    void createCubemp() {
        prepareEq2CubePipeline();

        cubeTex = _textureManager->createTexture(
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
                                  Shit::ImageLayout::GENERAL},
            Shit::ImageViewType::TYPE_CUBE, 0);
        cubeTex->prepare();

        //============
        // create descriptorset
        auto sampler = g_App->getSampler("linear");
        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, equirectangularTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {0, cubeTex->getImageView(), Shit::ImageLayout::GENERAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {eq2cubePipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
             &imageInfos[0]},
            {eq2cubePipelineInfo.descriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imageInfos[1]},
        };
        _device->UpdateDescriptorSets({writeSets, writeSets + std::size(writeSets)});

        // run pipeline
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            auto pipeline = dynamic_cast<Shit::ComputePipeline *>(eq2cubePipelineInfo.pipeline);
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, eq2cubePipelineInfo.pipeline});

            cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::COMPUTE, pipeline->GetCreateInfoPtr()->pLayout, 0,
                                           (uint32_t)eq2cubePipelineInfo.descriptorSets.size(),
                                           eq2cubePipelineInfo.descriptorSets.data()});
            cmdBuffer->Dispatch({SKYBOX_WIDTH, 6, 1});
            cmdBuffer->GenerateMipmap({cubeTex->getImage(), Shit::Filter::LINEAR, Shit::ImageLayout::GENERAL,
                                       Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL});
        });
    }
    void writeIrradianceDescriptorSets() {
        // create llm image
        irradianceSHLlmTex =
            _textureManager->createTexture(Shit::ImageCreateInfo{{},
                                                                 Shit::ImageType::TYPE_1D,
                                                                 Shit::Format::R32G32B32A32_SFLOAT,
                                                                 {32, 1, 1},
                                                                 1,
                                                                 1,
                                                                 Shit::SampleCountFlagBits::BIT_1,
                                                                 Shit::ImageTiling::OPTIMAL,
                                                                 Shit::ImageUsageFlagBits::STORAGE_BIT,
                                                                 Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                                                 Shit::ImageLayout::GENERAL},
                                           Shit::ImageViewType::TYPE_1D, 0);
        irradianceSHLlmTex->prepare();

        irradianceTex = _textureManager->createTexture(
            Shit::ImageCreateInfo{Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
                                  Shit::ImageType::TYPE_2D,
                                  Shit::Format::R8G8B8A8_UNORM,
                                  {64, 64, 1},
                                  1,
                                  6,
                                  Shit::SampleCountFlagBits::BIT_1,
                                  Shit::ImageTiling::OPTIMAL,
                                  Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                  Shit::ImageLayout::GENERAL},
            Shit::ImageViewType::TYPE_CUBE, 0);
        irradianceTex->prepare();

        // update sh llm descriptorsets
        auto linearSampler = getSampler("linear");
        Shit::DescriptorImageInfo imagesInfo[]{
            {linearSampler, equirectangularTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {nullptr, irradianceSHLlmTex->getImageView(), Shit::ImageLayout::GENERAL},
        };

        Shit::WriteDescriptorSet llmwrites[]{{irradianceSHLlmPipelineInfo.descriptorSets[0], 0, 0, 1,
                                              Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
                                             {irradianceSHLlmPipelineInfo.descriptorSets[0], 1, 0, 1,
                                              Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]}};
        _device->UpdateDescriptorSets(llmwrites);

        Shit::DescriptorImageInfo imagesInfo2[]{
            {nullptr, irradianceSHLlmTex->getImageView(), Shit::ImageLayout::GENERAL},
            {nullptr, irradianceTex->getImageView(), Shit::ImageLayout::GENERAL}};

        // update sh descriptorsets
        Shit::WriteDescriptorSet writes[]{
            {irradianceSHPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo2[0]},
            {irradianceSHPipelineInfo.descriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo2[1]},
        };
        _device->UpdateDescriptorSets(writes);
    }
    void generateIrradianceTex() {
        prepareSHLlmPipeline();
        prepareSHPipeline();
        writeIrradianceDescriptorSets();

        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            cmdBuffer->BindPipeline({Shit::PipelineBindPoint::COMPUTE, irradianceSHLlmPipelineInfo.pipeline});
            auto pipeline = dynamic_cast<Shit::ComputePipeline *>(irradianceSHLlmPipelineInfo.pipeline);
            cmdBuffer->BindDescriptorSets({
                Shit::PipelineBindPoint::COMPUTE,
                pipeline->GetCreateInfoPtr()->pLayout,
                0,
                (uint32_t)irradianceSHLlmPipelineInfo.descriptorSets.size(),
                irradianceSHLlmPipelineInfo.descriptorSets.data(),
            });
            cmdBuffer->Dispatch({1, 1, 1});

            Shit::ImageMemoryBarrier imageBarrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                                  Shit::AccessFlagBits::SHADER_READ_BIT,
                                                  Shit::ImageLayout::GENERAL,
                                                  Shit::ImageLayout::GENERAL,
                                                  ST_QUEUE_FAMILY_IGNORED,
                                                  ST_QUEUE_FAMILY_IGNORED,
                                                  irradianceSHLlmTex->getImage(),
                                                  {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};
            cmdBuffer->PipelineBarrier({Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                        Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                        {},
                                        0,
                                        0,
                                        0,
                                        0,
                                        1,
                                        &imageBarrier});

            cmdBuffer->BindPipeline({Shit::PipelineBindPoint::COMPUTE, irradianceSHPipelineInfo.pipeline});
            pipeline = dynamic_cast<Shit::ComputePipeline *>(irradianceSHPipelineInfo.pipeline);
            cmdBuffer->BindDescriptorSets({
                Shit::PipelineBindPoint::COMPUTE,
                pipeline->GetCreateInfoPtr()->pLayout,
                0,
                (uint32_t)irradianceSHPipelineInfo.descriptorSets.size(),
                irradianceSHPipelineInfo.descriptorSets.data(),
            });
            cmdBuffer->Dispatch({64, 6, 1});

            imageBarrier = Shit::ImageMemoryBarrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                                    Shit::AccessFlagBits::SHADER_READ_BIT,
                                                    Shit::ImageLayout::GENERAL,
                                                    Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                    cmdBuffer->GetCommandPool()->GetCreateInfoPtr()->queueFamilyIndex,
                                                    _graphicsQueue->GetFamilyIndex(),
                                                    irradianceTex->getImage(),
                                                    {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 6}};
            cmdBuffer->PipelineBarrier({Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                        Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                        {},
                                        0,
                                        0,
                                        0,
                                        0,
                                        1,
                                        &imageBarrier});
        });

        // takeScreenshot(_device, irradianceSHLlmTex->getImage(), Shit::ImageLayout::GENERAL);
        // takeScreenshot(_device, irradianceTex->getImage(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }
    void writeSkyboDescriptorSet() {
        Shit::DescriptorImageInfo imageInfo{getSampler("linear"), cubeTex->getImageView(),
                                            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
        Shit::WriteDescriptorSet writeSet{skyboxPipelineInfo.descriptorSets[0],         0,         0, 1,
                                          Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfo};
        _device->UpdateDescriptorSets({&writeSet, 1});
    }
    void writeRefectDescriptorSet() {
        auto sampler = getSampler("trilinear");

        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, irradianceTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            //{sampler, imageView, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {pipelineInfo.descriptorSets[0], 9, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, imageInfos},
        };
        _device->UpdateDescriptorSets(writeSets);
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

        cmdBuffer->SetViewport({0, 1, &viewport});
        cmdBuffer->SetScissor({0, 1, &scissor});

        // bind camera
        auto tempDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(skyboxPipelineInfo.pipeline);
        cmdBuffer->BindDescriptorSets(
            {Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &tempDescriptorSet});
        cmdBuffer->BindPipeline({Shit::PipelineBindPoint::GRAPHICS, skyboxPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets({Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 0,
                                       (uint32_t)skyboxPipelineInfo.descriptorSets.size(),
                                       skyboxPipelineInfo.descriptorSets.data()});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{36, 1, 0, 0});

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
        ImGui::Begin("image");
        ImGui::Image(equirectangularTex->getImageView(), ImVec2(300, 200));

        // ImGui::Text("lod edit");
        // auto materialData = pMaterialDataBlock->getUBOData();
        // static bool flag = false;
        // flag |= ImGui::InputFloat("lod", &materialData->roughness, 0.1);

        // if (flag)
        //{
        //	pMaterialDataBlock->update();
        //	flag = false;
        // }

        ImGui::End();
    }
    void prepare() override {
        loadAssets();
        createCubemp();
        generateIrradianceTex();

        prepareSkyboxPipeline();
        writeSkyboDescriptorSet();

        preparePipeline();
        writeRefectDescriptorSet();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)