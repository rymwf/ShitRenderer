#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "shadowcube/display.vert";
static auto fragShaderPath = "shadowcube/display.frag";
static auto showTexVertShaderPath = "shadowcube/showTex.vert";
static auto showTexFragShaderPath = "shadowcube/showTex.frag";
static auto shadowVertShaderPath = "shadowcube/shadow.vert";
static auto shadowGeomShaderPath = "shadowcube/shadow.geom";
static auto shadowFragShaderPath = "shadowcube/shadow.frag";

static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/sampleroom.obj";

#define SHADOW_MAP_WIDTH 1024

struct PipelineInfo {
    std::vector<Shader *> shaders;
    std::vector<Shit::DescriptorSet *> descriptorSets;
    Shit::Pipeline *pipeline;
};
struct UBODisplay {
    int renderMode{1};
    float normalBiasFactor{0.05};
    float shadowFilterRadius{1.};
};
struct ShowTexVar {
    int texLayer;
};

float depthBiasContantFactor = 0.;
float depthBiasSlopFactor = 2.;
float depthBiasClamp = 0.;

class LightController : public Behaviour {
    void prepareImpl() override { getParentTransform()->translate({5., 3., 0.}); }
    void updatePerFrameImpl(float frameDtMs) override {
        getParentTransform()->rotate({0, glm::radians(frameDtMs / 1000 * _anglePerSecond), 0}, TransformSpace::PARENT);
    }

    float _anglePerSecond = 3;

public:
    LightController(GameObject *parent) : Behaviour(parent) {}
    constexpr void setSpeed(float anglePerSecond) { _anglePerSecond = anglePerSecond; }
    constexpr float getSpeed() const { return _anglePerSecond; }
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;
    PipelineInfo showTexPipelineInfo;

    RenderTarget shadowRenderTarget;
    PipelineInfo shadowPipelineInfo;

    Light *light;
    LightController *lightController;
    Shit::Buffer *uboDisplayBuffer;
    UBODisplay uboDisplayData{};

    int viewMode = 0;
    ShowTexVar showTexVar{};

public:
    void loadAssets() {
        auto modelSrc = static_cast<Model *>(_modelManager->create(testModelPath));
        modelSrc->load();
        auto modelObj = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();

        for (auto it = modelSrc->meshBegin(), end = modelSrc->meshEnd(); it != end; ++it) {
            modelObj->addChild()->addComponent<MeshRenderer>(it->get());
        }
        modelObj->prepare();

        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>(LIGHT_SPHERE);
        lightController = light->getParent()->addComponent<LightController>();

        uboDisplayBuffer = _device->Create(
            Shit::BufferCreateInfo{
                {},
                sizeof(UBODisplay),
                Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
                Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT},
            &uboDisplayData);
    }
    void prepareShowTexPipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        showTexPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, showTexPipelineInfo.descriptorSets.data());

        // pipelina layout
        Shit::PushConstantRange range{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(ShowTexVar)};

        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{descriptorSetLayout};
        auto pipelineLayoutCI = Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                               descriptorSetLayouts.data(), 1, &range};

        auto pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        showTexPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                           buildShaderPath(_device, showTexVertShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                       static_cast<Shader *>(_shaderManager->create(
                                           buildShaderPath(_device, showTexFragShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : showTexPipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR,
                                           Shit::DynamicState::DEPTH_BIAS};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        showTexPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void prepareShadowRenderTarget() {
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // depth
            {depthFormat, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::AttachmentReference depthAttachment{0, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDesc[]{Shit::PipelineBindPoint::GRAPHICS, 0, 0, 0, 0, 0, &depthAttachment};

        shadowRenderTarget.renderPass = _device->Create(
            Shit::RenderPassCreateInfo{(uint32_t)attachmentDesc.size(), attachmentDesc.data(), 1, subpassDesc});
        shadowRenderTarget.clearValues = {Shit::ClearDepthStencilValue{1, 0}};
        // create depth attachment and framebuffer
        auto count = 1;
        auto depthImageCI = Shit::ImageCreateInfo{
            Shit::ImageCreateFlagBits::CUBE_COMPATIBLE_BIT,
            Shit::ImageType::TYPE_2D,
            depthFormat,
            {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH, 1},
            1,
            6,
            Shit::SampleCountFlagBits::BIT_1,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
        auto depthImageViewCI = Shit::ImageViewCreateInfo{nullptr,
                                                          Shit::ImageViewType::TYPE_CUBE,
                                                          depthFormat,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, 6}};
        for (int i = 0; i < count; ++i) {
            depthImageViewCI.pImage = shadowRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            shadowRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            shadowRenderTarget.frameBuffers.emplace_back(
                _device->Create(Shit::FramebufferCreateInfo{shadowRenderTarget.renderPass,
                                                            1,
                                                            &shadowRenderTarget.imageViews[i],
                                                            {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH},
                                                            6}));
        }
    }
    void prepareShadowPipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };
        auto pipelineLayoutCI =
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};

        auto pipelineLayout = _device->Create(pipelineLayoutCI);

        //===============================
        shadowPipelineInfo.shaders = {
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, shadowVertShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, shadowGeomShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "GEOM"}})),
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, shadowFragShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}})),
        };

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : shadowPipelineInfo.shaders) {
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

        Shit::DynamicState dynamicStates[]{
            Shit::DynamicState::VIEWPORT,
            Shit::DynamicState::SCISSOR,
            Shit::DynamicState::DEPTH_BIAS,
        };
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        bool isVK = (getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::VULKAN;

        shadowPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {1, &viewport, 1, &scissor},               // PipelineViewportStateCreateInfo
            {},                                        // PipelineTessellationStateCreateInfo
            {false, false, Shit::PolygonMode::FILL, isVK ? Shit::CullMode::FRONT : Shit::CullMode::BACK,
             Shit::FrontFace::COUNTER_CLOCKWISE, true, 0, 0, 0, 1.f},  // PipelineRasterizationStateCreateInfo
            {
                Shit::SampleCountFlagBits::BIT_1,
            },  // PipelineMultisampleStateCreateInfo
            {
                true,
                true,
                Shit::CompareOp::LESS,
            },  // PipelineDepthStencilStateCreateInfo
            {
                false, {},
                // 1,
                //&colorBlendAttachmentState,
            },  // PipelineColorBlendStateCreateInfo
            {
                (uint32_t)std::size(dynamicStates),
                dynamicStates,
            },  // PipelineDynamicStateCreateInfo
            pipelineLayout,
            shadowRenderTarget.renderPass,
            0});
    }
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {2, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {8, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        auto count = _swapchain->GetImageCount();
        std::vector<Shit::DescriptorSetLayout *> setLayouts(count, descriptorSetLayout);
        pipelineInfo.descriptorSets.resize(count);
        _descriptorPool->Allocate({(uint32_t)setLayouts.size(), setLayouts.data()}, pipelineInfo.descriptorSets.data());
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout, _cameraDescriptorSetLayout, _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,     descriptorSetLayout,
        };
        auto pipelineLayoutCI =
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()};

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
            true,
            Shit::BlendFactor::SRC_ALPHA,
            Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
            Shit::BlendOp::ADD,
            Shit::BlendFactor::SRC_ALPHA,
            Shit::BlendFactor::ONE_MINUS_SRC_ALPHA,
            Shit::BlendOp::ADD,
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
                Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT};

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
        Shit::DescriptorBufferInfo bufferInfo{uboDisplayBuffer, 0, sizeof(UBODisplay)};

        // write
        Shit::DescriptorImageInfo imageInfo{getSampler("linear"), shadowRenderTarget.imageViews[0],
                                            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

        std::vector<Shit::WriteDescriptorSet> writeSets;
        auto count = _swapchain->GetImageCount();
        for (int i = 0; i < count; ++i) {
            writeSets.emplace_back(Shit::WriteDescriptorSet{pipelineInfo.descriptorSets[i], 2, 0, 1,
                                                            Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo});
            writeSets.emplace_back(Shit::WriteDescriptorSet{pipelineInfo.descriptorSets[i], 8, 0, 1,
                                                            Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfo});
        };
        _device->UpdateDescriptorSets(writeSets);
    }
    void writeShowTexDescriptorSets() {
        auto sampler = getSampler("linear");

        auto imageView =
            _device->Create(Shit::ImageViewCreateInfo{shadowRenderTarget.imageViews[0]->GetCreateInfoPtr()->pImage,
                                                      Shit::ImageViewType::TYPE_2D_ARRAY,
                                                      shadowRenderTarget.imageViews[0]->GetCreateInfoPtr()->format,
                                                      {},
                                                      {Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, 6}});

        Shit::DescriptorImageInfo imageInfo{sampler, imageView, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
        std::vector<Shit::WriteDescriptorSet> writeSets{
            {showTexPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imageInfo},
        };
        _device->UpdateDescriptorSets(writeSets);
    }
    void updateUBODisplayBuffer() {
        void *data;
        uboDisplayBuffer->MapMemory(0, sizeof(UBODisplay), &data);
        memcpy(data, &uboDisplayData, sizeof(UBODisplay));
        uboDisplayBuffer->UnMapMemory();
    }
    void setDefaultRenderPassClearValue() override {
        Shit::ClearColorValueFloat clearColor = {0., 0., 0., 1};
        Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
        _defaultRenderTarget.clearValues = {clearColor, clearDepthStencil};
        if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
            _defaultRenderTarget.clearValues.emplace_back(clearColor);
        }
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

        Shit::Viewport shadow_viewport{0, 0, SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH, 0, 1};
        if ((getRendererVersion() & Shit::RendererVersion::TypeBitmask) == Shit::RendererVersion::VULKAN) {
            shadow_viewport.y = -shadow_viewport.height - shadow_viewport.y;
            shadow_viewport.height = -shadow_viewport.height;
        }
        Shit::Rect2D shadow_scissor{{}, {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH}};

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
        // draw shadow
        auto shadowPipeline = dynamic_cast<Shit::GraphicsPipeline *>(shadowPipelineInfo.pipeline);
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{shadowRenderTarget.renderPass,
                                                             shadowRenderTarget.frameBuffers[0],
                                                             {{}, {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH}},
                                                             (uint32_t)shadowRenderTarget.clearValues.size(),
                                                             shadowRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        // bind camera
        auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, shadowPipeline->GetCreateInfoPtr()->pLayout, 1, 1, &descriptorSet});

        // bind light
        descriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, shadowPipeline->GetCreateInfoPtr()->pLayout, 3, 1, &descriptorSet});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &shadow_viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &shadow_scissor});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, shadowPipelineInfo.pipeline});

        cmdBuffer->SetDepthBias({depthBiasContantFactor, depthBiasClamp, depthBiasSlopFactor});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), shadowPipeline->GetCreateInfoPtr()->pLayout);
        cmdBuffer->EndRenderPass();

        // draw scene
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        if (viewMode == 0) {
            auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo.pipeline);

            // bind light
            descriptorSet = light->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &descriptorSet});

            // bind camera
            descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 1, 1, &descriptorSet});

            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                       pipeline->GetCreateInfoPtr()->pLayout, 4, 1,
                                                                       &pipelineInfo.descriptorSets[imageIndex]});

            cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline});

            for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);
        } else if (viewMode == 1) {
            auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(showTexPipelineInfo.pipeline);
            cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipeline});
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                       pipeline->GetCreateInfoPtr()->pLayout, 0, 1,
                                                                       &showTexPipelineInfo.descriptorSets[0]});
            cmdBuffer->PushConstants({pipeline->GetCreateInfoPtr()->pLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0,
                                      sizeof(ShowTexVar), &showTexVar});
            cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
        }
        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("editor");
        // camera pos
        {
            ImGui::Text("camera edit");
            static float dist = 10.;
            ImGui::InputFloat("dist", &dist);

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
            static bool lightPause = false;
            if (ImGui::Checkbox("lightpause", &lightPause)) {
                if (lightPause)
                    lightController->pause();
                else
                    lightController->run();
            }
            static float v = lightController->getSpeed();
            if (ImGui::InputFloat("rotate speed", &v, 1.)) {
                lightController->setSpeed(v);
            }

            _needUpdateDefaultCommandBuffer |=
                ImGui::InputFloat("depthBiasContantFactor", &depthBiasContantFactor, 1.f);
            _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("depthBiasClamp", &depthBiasClamp, 1.f);
            _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("depthBiasSlopFactor", &depthBiasSlopFactor, 1.f);

            _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("scene", &viewMode, 0);
            if (viewMode == 0) {
                static bool updateUBOBuffer = false;
                static bool shadowOnly = false;
                static int renderMode = 1;
                updateUBOBuffer |= ImGui::Checkbox("shadowonly", &shadowOnly);
                updateUBOBuffer |= ImGui::RadioButton("hardshardow", &renderMode, 1);
                updateUBOBuffer |= ImGui::RadioButton("PCF", &renderMode, 2);
                updateUBOBuffer |= ImGui::RadioButton("PCSS", &renderMode, 3);
                updateUBOBuffer |= ImGui::RadioButton("VSM", &renderMode, 4);

                updateUBOBuffer |= ImGui::SliderFloat("normalBiasFactor", &uboDisplayData.normalBiasFactor, 0.f, 0.1f);
                updateUBOBuffer |=
                    ImGui::SliderFloat("shadowFilterRadius", &uboDisplayData.shadowFilterRadius, 0.f, 10.f);
                if (updateUBOBuffer) {
                    uboDisplayData.renderMode = shadowOnly ? -renderMode : renderMode;
                    updateUBODisplayBuffer();
                }
            }
            _needUpdateDefaultCommandBuffer |= ImGui::RadioButton("depth", &viewMode, 1);
            if (viewMode == 1) {
                _needUpdateDefaultCommandBuffer |= ImGui::SliderInt("texLayer", &showTexVar.texLayer, 0, 6);
            }
        }
        ImGui::End();
    }
    void prepare() override {
        loadAssets();
        preparePipeline();
        prepareShadowRenderTarget();
        prepareShadowPipeline();
        prepareShowTexPipeline();

        writeShowTexDescriptorSets();
        writeDescriptorSets();
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)