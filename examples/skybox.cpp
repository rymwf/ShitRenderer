#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "skybox/reflect.vert";
static auto fragShaderPath = "skybox/reflect.frag";
static auto eq2CubeCompShaderName = "common/equirectangular2cube.comp";
static auto cubemapMipmapCompShaderName = "common/cubemapmipmap.comp";
static auto skyboxVertShaderPath = "skybox/skybox.vert";
static auto skyboxFragShaderPath = "skybox/skybox.frag";

// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj";
static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/sampleroom.obj";
static auto imagePath = SHIT_SOURCE_DIR "/assets/images/limpopo_golf_course_4k.hdr";

#define SKYBOX_WIDTH 1024

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    Shit::PipelineLayout *pipelineLayout;
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;
    GameObject *modelObject;
    Light *light;
    MaterialDataBlock *pMaterialDataBlock;

    PipelineInfo eq2cubePipelineInfo;
    Shit::DescriptorSet *eq2cubeDescriptorSet;

    PipelineInfo cubemapMipmapPipelineInfo;

    PipelineInfo skyboxPipelineInfo;
    Shit::DescriptorSet *skyboxDescriptorSet;

    Image *equirectangularImage;
    Texture *equirectangularTex;
    Texture *cubeTex;
    Shit::DescriptorSetLayout *reflectDescriptorSetLayout;
    Shit::DescriptorSet *reflectDescriptorSet;

public:
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding binding = {9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1,
                                                    Shit::ShaderStageFlagBits::FRAGMENT_BIT};
        reflectDescriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &binding});

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout, _cameraDescriptorSetLayout, _materialDescriptorSetLayout,
            _lightDescriptorSetLayout, reflectDescriptorSetLayout};

        pipelineInfo.pipelineLayout = _device->Create(
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
            pipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        // modelObject->getComponent<Transform>()
        //	->pitch(glm::radians(-90.f))
        //	->roll(glm::radians(-90.f));

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            auto meshRenderer = child->addComponent<MeshRenderer>(it->get());
            pMaterialDataBlock = meshRenderer->getPrimitiveRenderer(0)->getMaterialDataBlock(0);
        }
        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-90.f));

        // load image
        equirectangularImage = static_cast<Image *>(_imageManager->create(imagePath));
        equirectangularTex = _textureManager->createTexture(equirectangularImage);
        equirectangularTex->prepare();
        equirectangularTex->getImage()->GenerateMipmap(Shit::Filter::LINEAR,
                                                       Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                       Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }
    void createEq2CubePipeline() {
        //==============
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            //
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        eq2cubePipelineInfo.pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        eq2cubePipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, eq2CubeCompShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        eq2cubePipelineInfo.shaders[0]->load();

        // generate cubemap pipeline
        uint32_t constantIDs0[] = {COSTANT_ID_COMPUTE_LOCAL_SIZE_X};
        uint32_t constantValues0[] = {SKYBOX_WIDTH};  // cubemap width

        Shit::PipelineShaderStageCreateInfo shaderStageCI{eq2cubePipelineInfo.shaders[0]->getStage(),
                                                          eq2cubePipelineInfo.shaders[0]->getHandle(),
                                                          eq2cubePipelineInfo.shaders[0]->getEntryName().data(),
                                                          {1, constantIDs0, constantValues0}};

        eq2cubePipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, eq2cubePipelineInfo.pipelineLayout});
    }
    void createCubemp() {
        createEq2CubePipeline();

        cubeTex = _textureManager->createTexture(
            Shit::ImageCreateInfo{Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
                                  Shit::ImageType::TYPE_2D,
                                  Shit::Format::R32G32B32A32_SFLOAT,
                                  {SKYBOX_WIDTH, SKYBOX_WIDTH, 1},
                                  0,  // allocate memory for mipmap
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
        g_App->getDescriptorPool()->Allocate(
            Shit::DescriptorSetAllocateInfo{1, &eq2cubePipelineInfo.pipelineLayout->GetCreateInfoPtr()->pSetLayouts[0]},
            &eq2cubeDescriptorSet);
        auto sampler = g_App->getSampler("linear");
        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, equirectangularTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {0, cubeTex->getImageView(), Shit::ImageLayout::GENERAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {eq2cubeDescriptorSet, 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
            {eq2cubeDescriptorSet, 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imageInfos[1]},
        };
        _device->UpdateDescriptorSets({writeSets, writeSets + std::size(writeSets)});

        // run pipeline
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, eq2cubePipelineInfo.pipeline});

            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::COMPUTE, eq2cubePipelineInfo.pipelineLayout, 0, 1, &eq2cubeDescriptorSet});
            cmdBuffer->Dispatch({SKYBOX_WIDTH, 6, 1});

            // transfer imagelayout to shder readonly
            Shit::ImageMemoryBarrier barrier{
                Shit::AccessFlagBits::SHADER_WRITE_BIT,
                Shit::AccessFlagBits::SHADER_READ_BIT,
                Shit::ImageLayout::GENERAL,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                cmdBuffer->GetCommandPool()->GetCreateInfoPtr()->queueFamilyIndex,
                _graphicsQueue->GetFamilyIndex(),
                cubeTex->getImage(),
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, cubeTex->getImage()->GetCreateInfoPtr()->arrayLayers}};
            cmdBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                 Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                                 {},
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 0,
                                                                 1,
                                                                 &barrier});
            // cmdBuffer->GenerateMipmap({cubeTex->getImage(),
            //						   Shit::Filter::LINEAR,
            //						   Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            //						   Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL});
        });
        // cubeTex->getImage()->GenerateMipmap(
        //	Shit::Filter::LINEAR,
        //	Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        //	Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
    }
    void createCubemapMipmapPipeline() {
        //==============
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            //
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };

        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        cubemapMipmapPipelineInfo.pipelineLayout =
            _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        cubemapMipmapPipelineInfo.shaders = {static_cast<Shader *>(
            _shaderManager->create(buildShaderPath(_device, cubemapMipmapCompShaderName, _rendererVersion),
                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        cubemapMipmapPipelineInfo.shaders[0]->load();
        Shit::PipelineShaderStageCreateInfo shaderStageCI{cubemapMipmapPipelineInfo.shaders[0]->getStage(),
                                                          cubemapMipmapPipelineInfo.shaders[0]->getHandle(),
                                                          cubemapMipmapPipelineInfo.shaders[0]->getEntryName().data()};

        cubemapMipmapPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, cubemapMipmapPipelineInfo.pipelineLayout});
    }
    void generateCubemapMipmap() {
        createCubemapMipmapPipeline();

        auto sampler = getSampler("linear");
        uint32_t width = cubeTex->getImage()->GetCreateInfoPtr()->extent.width;
        uint32_t mipLevels = cubeTex->getImage()->GetCreateInfoPtr()->mipLevels;

        auto descriptorSetLayout = cubemapMipmapPipelineInfo.pipelineLayout->GetCreateInfoPtr()->pSetLayouts[0];
        std::vector<Shit::DescriptorSetLayout const *> descriptorSetLayouts(mipLevels - 1, descriptorSetLayout);
        std::vector<Shit::DescriptorSet *> cubemapMipmapDescriptorSets(mipLevels - 1);
        _descriptorPool->Allocate({mipLevels - 1, descriptorSetLayouts.data()}, cubemapMipmapDescriptorSets.data());

        std::vector<Shit::ImageView *> cubeTexImageViews;
        for (uint32_t i = 0; i < mipLevels; ++i) {
            cubeTexImageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{cubeTex->getImage(),
                                                          Shit::ImageViewType::TYPE_CUBE,
                                                          cubeTex->getImage()->GetCreateInfoPtr()->format,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0,
                                                           cubeTex->getImage()->GetCreateInfoPtr()->arrayLayers}}));
        }
        for (uint32_t i = 1; i < mipLevels; ++i) {
            Shit::DescriptorImageInfo imagesInfo[]{
                {
                    sampler,
                    cubeTexImageViews[i - 1],
                    Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                },
                {
                    0,
                    cubeTexImageViews[i],
                    Shit::ImageLayout::GENERAL,
                },
            };
            Shit::WriteDescriptorSet writeSets[]{
                {cubemapMipmapDescriptorSets[i - 1], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                 &imagesInfo[0]},
                {cubemapMipmapDescriptorSets[i - 1], 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]},
            };
            _device->UpdateDescriptorSets(writeSets);
        }

        // run pipeline
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *cmdBuffer) {
            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, cubemapMipmapPipelineInfo.pipeline});

            for (uint32_t i = 1; i < mipLevels; ++i) {
                width /= 2;

                cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE,
                                                                           cubemapMipmapPipelineInfo.pipelineLayout, 0,
                                                                           1, &cubemapMipmapDescriptorSets[i - 1]});
                cmdBuffer->Dispatch({width, 6, 1});

                // transfer imagelayout to shder readonly
                Shit::ImageMemoryBarrier barrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                                 Shit::AccessFlagBits::SHADER_READ_BIT,
                                                 Shit::ImageLayout::GENERAL,
                                                 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                 cmdBuffer->GetCommandPool()->GetCreateInfoPtr()->queueFamilyIndex,
                                                 _graphicsQueue->GetFamilyIndex(),
                                                 cubeTex->getImage(),
                                                 {Shit::ImageAspectFlagBits::COLOR_BIT, i, 1, 0,
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
            }
        });
    }

    void createRefectDescriptorSet() {
        _descriptorPool->Allocate({1, &reflectDescriptorSetLayout}, &reflectDescriptorSet);
        // auto sampler = getSampler("nearest");
        auto sampler = getSampler("trilinear");
        // write
        Shit::DescriptorImageInfo imageInfos[]{
            {sampler, cubeTex->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::WriteDescriptorSet writeSets[]{
            {reflectDescriptorSet, 9, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfos[0]},
        };
        _device->UpdateDescriptorSets({writeSets, writeSets + std::size(writeSets)});
    }
    void createSkyboxPipeline() {
        // pipeline layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };

        Shit::DescriptorSetLayout *descriptorSetLayouts[]{
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings}),
            _cameraDescriptorSetLayout,
        };
        skyboxPipelineInfo.pipelineLayout =
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
            skyboxPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void createSkyboDescriptorSet() {
        _descriptorPool->Allocate(
            Shit::DescriptorSetAllocateInfo{1, &skyboxPipelineInfo.pipelineLayout->GetCreateInfoPtr()->pSetLayouts[0]},
            &skyboxDescriptorSet);

        Shit::DescriptorImageInfo imageInfo{getSampler("linear"), cubeTex->getImageView(),
                                            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
        Shit::WriteDescriptorSet writeSet{skyboxDescriptorSet, 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                          &imageInfo};
        _device->UpdateDescriptorSets({&writeSet, 1});
    }

    void prepare() override {
        loadAssets();
        createCubemp();
        generateCubemapMipmap();

        preparePipeline();
        createRefectDescriptorSet();
        createSkyboxPipeline();
        createSkyboDescriptorSet();
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

        // bind camera
        auto tempDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, skyboxPipelineInfo.pipelineLayout, 1, 1, &tempDescriptorSet});
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, skyboxPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, skyboxPipelineInfo.pipelineLayout, 0, 1, &skyboxDescriptorSet});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{36, 1, 0, 0});

        //======================
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipelineLayout, 1, 1, &tempDescriptorSet});

        // bind light
        tempDescriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipelineLayout, 3, 1, &tempDescriptorSet});

        // bind cube
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipelineLayout, 4, 1, &reflectDescriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipelineInfo.pipelineLayout);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("image");
        ImGui::Image(equirectangularTex->getImageView(), ImVec2(300, 200));

        ImGui::Text("lod edit");
        auto materialData = pMaterialDataBlock->getUBOData();
        static bool flag = false;
        flag |= ImGui::InputFloat("lod", &materialData->roughness, 0.1);

        if (flag) {
            pMaterialDataBlock->update();
            flag = false;
        }
        ImGui::End();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)