#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath0 = "translucency/write.vert";
static auto fragShaderPath0 = "translucency/write.frag";
static auto vertShaderPath1 = "translucency/read.vert";
static auto fragShaderPath1 = "translucency/read.frag";

static const char *testModelPaths[]{
    SHIT_SOURCE_DIR "/assets/models/obj/chinesedragon.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/teapot.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/sphere_ico.obj",
};
static char const *imagePaths[]{
    SHIT_SOURCE_DIR "/assets/images/ennis.hdr",
    SHIT_SOURCE_DIR "/assets/images/Footprint_Court_2k.hdr",
    SHIT_SOURCE_DIR "/assets/images/grace-new.hdr",
};

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

struct PCPara {
    float extinction[3]{1, 0, 0};
    float extinctionFactor{1};
    float indexOfRefraction{1.5};
    float roughness{0.1};
    int fresnelEffect{1};
    int enableReflection{1};
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
    PipelineInfo translucencyPipelineInfo;
    PipelineInfo skyboxPipelineInfo;

    RenderTarget skyboxRenderTarget;

    std::vector<Image *> equirectangularImages;

    Image *envBRDFImage;
    Texture *envBRDFTex;
    Shit::Sampler *envBRDFTexSampler;

    GameObject *chineseDragon;

    ModelController *modelController;

    std::vector<Shit::Image *> counterImages;
    std::vector<Shit::ImageView *> counterImageViews;
    std::vector<Shit::Image *> outImages;
    std::vector<Shit::ImageView *> outImageViews;

    PCPara uPC{};

public:
    void loadAssets() {
        std::vector<Model *> models;
        for (auto e : testModelPaths) {
            models
                .emplace_back(static_cast<Model *>(_modelManager->create(e)))
                //->enableGenerateOctreeFaces(8)
                //->enableCalcDistanceAlongNormal()
                ->load();
        }

        chineseDragon = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        // modelController = modelObject->addComponent<ModelController>();
        _editorCamera->setFarPlane(10);
        modelController = _editorCamera->getParent()->addComponent<ModelController>();

        for (auto it = models[0]->meshBegin(), end = models[0]->meshEnd(); it != end; ++it) {
            chineseDragon->addChild()->addComponent<MeshRenderer>(it->get());
        }

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

        for (auto e : imagePaths) {
            equirectangularImages.emplace_back(static_cast<Image *>(_imageManager->create(e)));
        }

        _environment = std::make_unique<Environment>(equirectangularImages[0]);
        _environment->prepare();

        //
        auto frameCount = _swapchain->GetImageCount();
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
                    Shit::Format::R32G32B32A32_UINT,
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
                                                          Shit::Format::R32G32B32A32_UINT,
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
        auto frameCount = _swapchain->GetImageCount();
        for (int i = 0; i < frameCount; ++i) {
            _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            frameBufferCI.pAttachments = &_defaultRenderTarget.imageViews[i * frameBufferCI.attachmentCount];
            _defaultRenderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
        }
    }

    void preparePipelineInfos() {
        auto frameCount = _swapchain->GetImageCount();
        {
            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
                {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto setLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});
            std::vector<Shit::DescriptorSetLayout *> tempSetLayouts(frameCount, setLayout);

            // pipelineInfo 0
            translucencyPipelineInfo.descriptorSets.resize(frameCount);
            _descriptorPool->Allocate({(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
                                      translucencyPipelineInfo.descriptorSets.data());
            std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
                _transformDescriptorSetLayout,
                _cameraDescriptorSetLayout,
                _materialDescriptorSetLayout,
                _lightDescriptorSetLayout,
                setLayout,
            };

            translucencyPipelineInfo.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

            //===============================
            translucencyPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
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
                {8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
                {9, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto setLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{(uint32_t)std::size(bindings), bindings});
            std::vector<Shit::DescriptorSetLayout *> tempSetLayouts(frameCount, setLayout);

            // pipelineInfo 1
            skyboxPipelineInfo.descriptorSets.resize(frameCount);
            _descriptorPool->Allocate({(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
                                      skyboxPipelineInfo.descriptorSets.data());

            std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
                setLayout,
                _cameraDescriptorSetLayout,
                _materialDescriptorSetLayout,
                _lightDescriptorSetLayout,
            };

            Shit::PushConstantRange range{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0, 0, sizeof(PCPara)};

            skyboxPipelineInfo.pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{
                (uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), 1, &range});

            //===============================
            skyboxPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                              buildShaderPath(_device, vertShaderPath1, _rendererVersion),
                                              ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                          static_cast<Shader *>(_shaderManager->create(
                                              buildShaderPath(_device, fragShaderPath1, _rendererVersion),
                                              ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};
        }
    }
    void createPipelineInfo0() {
        auto shaderStageCIs = translucencyPipelineInfo.getStageCIs();

        Shit::VertexBindingDescription vertexBindings[]{
            {0, (uint32_t)sizeof(float) * 3, 0},
            {1, (uint32_t)sizeof(float) * 3, 0},
            {2, (uint32_t)sizeof(float) * 2, 0},
            //{3, (uint32_t)sizeof(float), 0},
        };
        Shit::VertexAttributeDescription attributeBindings[]{
            {0, 0, Shit::Format::R32G32B32_SFLOAT, 0},
            {1, 1, Shit::Format::R32G32B32_SFLOAT, 0},
            {2, 2, Shit::Format::R32G32_SFLOAT, 0},
            //{3, 3, Shit::Format::R32_SFLOAT, 0},
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

        translucencyPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            translucencyPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void createPipelineInfo1() {
        auto shaderStageCIs = skyboxPipelineInfo.getStageCIs();

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
            skyboxPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            1});
    }
    void updateDescriptorSets() {
        auto frameCount = _swapchain->GetImageCount();
        auto attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;

        uint32_t imageCount = 4;
        std::vector<Shit::DescriptorImageInfo> imagesInfo(frameCount * imageCount);
        std::vector<Shit::WriteDescriptorSet> writes;

        auto trilinearSampler = getSampler("trilinear");

        for (uint32_t i = 0; i < frameCount; ++i) {
            imagesInfo[i * imageCount + 0] = {0, counterImageViews[i], Shit::ImageLayout::GENERAL};
            imagesInfo[i * imageCount + 1] = {0, outImageViews[i], Shit::ImageLayout::GENERAL};
            imagesInfo[i * imageCount + 2] = {trilinearSampler, _environment->getEnvTex()->getImageView(),
                                              Shit::ImageLayout::GENERAL};
            imagesInfo[i * imageCount + 3] = {envBRDFTexSampler, envBRDFTex->getImageView(),
                                              Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

            writes.emplace_back(Shit::WriteDescriptorSet{translucencyPipelineInfo.descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE,
                                                         &imagesInfo[i * imageCount]});
            writes.emplace_back(Shit::WriteDescriptorSet{translucencyPipelineInfo.descriptorSets[i], 1, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE,
                                                         &imagesInfo[i * imageCount + 1]});

            writes.emplace_back(Shit::WriteDescriptorSet{skyboxPipelineInfo.descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE,
                                                         &imagesInfo[i * imageCount]});
            writes.emplace_back(Shit::WriteDescriptorSet{skyboxPipelineInfo.descriptorSets[i], 1, 0, 1,
                                                         Shit::DescriptorType::STORAGE_IMAGE,
                                                         &imagesInfo[i * imageCount + 1]});
            writes.emplace_back(Shit::WriteDescriptorSet{skyboxPipelineInfo.descriptorSets[i], 8, 0, 1,
                                                         Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                         &imagesInfo[i * imageCount + 2]});
            writes.emplace_back(Shit::WriteDescriptorSet{skyboxPipelineInfo.descriptorSets[i], 9, 0, 1,
                                                         Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                         &imagesInfo[i * imageCount + 3]});
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


        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
        auto pipeline0 = dynamic_cast<Shit::GraphicsPipeline *>(translucencyPipelineInfo.pipeline);
        auto pipeline1 = dynamic_cast<Shit::GraphicsPipeline *>(skyboxPipelineInfo.pipeline);

        Shit::ImageSubresourceRange range{Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1};
        cmdBuffer->ClearColorImage(
            {counterImages[imageIndex], Shit::ImageLayout::GENERAL, Shit::ClearColorValueUint32{0}, 1, &range});

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
                                                                   &translucencyPipelineInfo.descriptorSets[imageIndex]});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline0->GetCreateInfoPtr()->pLayout, 2);

        cmdBuffer->NextSubpass(Shit::SubpassContents::INLINE);
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline1});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline1->GetCreateInfoPtr()->pLayout, 1, 1, &cameraDescriptorSet});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline1->GetCreateInfoPtr()->pLayout, 0, 1,
                                                                   &skyboxPipelineInfo.descriptorSets[imageIndex]});

        cmdBuffer->PushConstants(
            {skyboxPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0, sizeof(PCPara), &uPC});

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

        static int imageIndex = 0;
        const char *eqImageName = imagePaths[imageIndex];  // Label to preview before opening the combo
                                                         // (technically it could be anything)
        if (ImGui::BeginCombo("skybox image", eqImageName)) {
            for (int i = 0; i < std::size(imagePaths); ++i) {
                const bool is_selected = imageIndex == i;
                if (ImGui::Selectable(imagePaths[i], is_selected)) {
                    imageIndex = i;
                    _environment->setSkyBoxEquirectangular(equirectangularImages[i]);
                    _needUpdateDefaultCommandBuffer = true;
                }
            }
            ImGui::EndCombo();
        }

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

        ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreviewHalf |
                                         ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_AlphaBar;
        _needUpdateDefaultCommandBuffer |=
            ImGui::ColorEdit3("extinction", uPC.extinction, ImGuiColorEditFlags_NoLabel | misc_flags);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("extinction", &uPC.extinctionFactor, 0, 100);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("indexOfRefraction", &uPC.indexOfRefraction, 0, 3);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("roughness", &uPC.roughness, 0, 1);

        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("fresnelEffect", (bool *)&uPC.fresnelEffect);
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enableReflection", (bool *)&uPC.enableReflection);

        ImGui::End();
    }
    void recreateSwapchainListener() {
        auto frameCount = _swapchain->GetImageCount();
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