#include "common/appbase.h"
#include "common/entry.h"
static auto writeVertShaderPath = "fxaa/write.vert";
static auto writeFragShaderPath = "fxaa/write.frag";
static auto readVertShaderPath = "fxaa/read.vert";
static auto readFragShaderPath = "fxaa/read.frag";

static auto postProcessFragShaderPath = "fxaa/postProcess.frag";

static auto shadowVertShaderPath = "shadow/shadow.vert";
static auto shadowGeomShaderPath = "shadow/shadow.geom";
static auto shadowFragShaderPath = "shadow/shadow.frag";

static const char *testModelPaths[]{
    SHIT_SOURCE_DIR "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/cube.obj",
    SHIT_SOURCE_DIR "/assets/models/obj/plane.obj",
};
// static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/cube.obj";
//  static auto testModelPath = SHIT_SOURCE_DIR
//  "/assets/models/obj/sampleroom.obj";
#define COLOR_ATTACHMENT_NUM 5
#define SHADOW_MAP_WIDTH 1024

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    Shit::PipelineLayout *pipelineLayout;
    std::vector<Shit::DescriptorSet *> descriptorSets;
};

struct UBORead {
    int renderMode;
    int sampleCount;
};

struct PostProcessPC {
    int fxaa{1};
    float fxaaThreashold{0.5};
    float fxaaThreasholdMin{0.04};
    float fxaaSharpness{50.};
    int highlightArea{};
};

float depthBiasContantFactor = 2.;
float depthBiasSlopFactor = 2.;
float depthBiasClamp = 0.;

class LightController : public Behaviour {
    void prepareImpl() override { getParentTransform()->translate({0, 3, 0}); }
    void updatePerFrameImpl(float frameDtMs) override {
        getParentTransform()->rotate({0, glm::radians(frameDtMs / 1000 * _anglePerSecond), 0});
    }

    float _anglePerSecond = 10;

public:
    LightController(GameObject *parent) : Behaviour(parent) {}
    constexpr void setSpeed(float anglePerSecond) { _anglePerSecond = anglePerSecond; }
    constexpr float getSpeed() const { return _anglePerSecond; }
};

class Hello : public AppBase {
    PipelineInfo writePipelineInfo;
    PipelineInfo readPipelineInfo;

    RenderTarget renderTarget;
    PipelineInfo postProcessPipelineInfo;

    std::vector<RenderTarget> shadowRenderTargets;
    PipelineInfo shadowPipelineInfo;

    UBORead uboRead{};
    PostProcessPC postProcessPC{};

    GameObject *modelObject;
    GameObject *lightRoot;
    std::vector<Light *> lights;
    LightController *lightController;

    std::vector<Shit::DescriptorSet *> shadowDescriptorSets;

public:
    void loadAssets() {
        std::vector<Model *> modelSrcs;
        for (auto p : testModelPaths) {
            modelSrcs.emplace_back(static_cast<Model *>(_modelManager->create(p)))->load();
        }

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        for (auto it = modelSrcs[0]->meshBegin(), end = modelSrcs[0]->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
        }
        for (auto it = modelSrcs[1]->meshBegin(), end = modelSrcs[1]->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
            child->getComponent<Transform>()->scale({10, 1, 10})->translate({0, -1, 0});
        }

        // add directional light
        lightRoot = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        lightController = lightRoot->addComponent<LightController>();
        for (int i = 0; i < 1; ++i) {
            auto l = lights.emplace_back(lightRoot->addChild()->addComponent<Light>(LIGHT_SPOT));
            l->getParentTransform()
                ->pitch(glm::radians(-45.f))
                ->translate({0, 0, 3})
                ->yaw(2.1 * i, TransformSpace::PARENT);
        }
        lights[0]->getSSBOData()->color = glm::vec3(1, 1, 1);
        // lights[1]->getSSBOData()->color = glm::vec3(1, 1, 0);
        // lights[2]->getSSBOData()->color = glm::vec3(1, 0, 1);
    }
    void createShadowRenderTargets() {
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

        shadowRenderTargets.resize(lights.size());
        auto renderPass = _device->Create(
            Shit::RenderPassCreateInfo{(uint32_t)attachmentDesc.size(), attachmentDesc.data(), 1, subpassDesc});
        for (auto &&shadowRenderTarget : shadowRenderTargets) {
            shadowRenderTarget.renderPass = renderPass;
            shadowRenderTarget.clearValues = {Shit::ClearDepthStencilValue{1, 0}};
            // create depth attachment and framebuffer
            auto count = 1;
            auto depthImageCI = Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                depthFormat,
                {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH, 1},
                1,
                1,
                Shit::SampleCountFlagBits::BIT_1,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            auto depthImageViewCI = Shit::ImageViewCreateInfo{nullptr,
                                                              Shit::ImageViewType::TYPE_2D_ARRAY,
                                                              depthFormat,
                                                              {},
                                                              {Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, 1}};

            depthImageViewCI.pImage = shadowRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            shadowRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            shadowRenderTarget.frameBuffers.emplace_back(
                _device->Create(Shit::FramebufferCreateInfo{shadowRenderTarget.renderPass,
                                                            1,
                                                            &shadowRenderTarget.imageViews[0],
                                                            {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH},
                                                            1}));
        }
    }
    void createShadowPipeline() {
        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };
        shadowPipelineInfo.pipelineLayout = _device->Create(
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});
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
             Shit::FrontFace::COUNTER_CLOCKWISE, true, 10, 2, 2, 1.f},  // PipelineRasterizationStateCreateInfo
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
            shadowPipelineInfo.pipelineLayout,
            shadowRenderTargets[0].renderPass,
            0});
    }
    void createRenderPass() {
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // deferred render target
            {Shit::Format::R8G8B8A8_UNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
             Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE, Shit::ImageLayout::UNDEFINED,
             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // write geometry pass attachments
            // postion
            {Shit::Format::R16G16B16A16_SFLOAT, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // albedo
            {Shit::Format::R16G16B16A16_UNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // normal
            {Shit::Format::R16G16B16A16_SNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // MetallicRoughnessAO
            {Shit::Format::R16G16B16A16_UNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // Emission
            {Shit::Format::R16G16B16A16_UNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // depth
            {depthFormat, _sampleCount, Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
             Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE, Shit::ImageLayout::UNDEFINED,
             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::AttachmentReference colorAttachments[]{
            {1, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL}, {2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            {3, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL}, {4, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            {5, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference depthAttachment{6, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::AttachmentReference inputAttachments1[]{
            {1, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, {2, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {3, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL}, {4, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {5, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::AttachmentReference shadingAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDescs[]{
            {Shit::PipelineBindPoint::GRAPHICS, 0, 0, (uint32_t)std::size(colorAttachments), colorAttachments, 0,
             &depthAttachment},
            {Shit::PipelineBindPoint::GRAPHICS, (uint32_t)std::size(inputAttachments1), inputAttachments1, 1,
             &shadingAttachment},
        };
        Shit::SubpassDependency subpassDependencies[]{
            {ST_SUBPASS_EXTERNAL,
             0,
             Shit::PipelineStageFlagBits::TOP_OF_PIPE_BIT,
             Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
                 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
             {},
             Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT},
            {0, 1,
             Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT |
                 Shit::PipelineStageFlagBits::LATE_FRAGMENT_TESTS_BIT,
             Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
             Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT |
                 Shit::AccessFlagBits::DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
             Shit::AccessFlagBits::SHADER_READ_BIT},
            {1, ST_SUBPASS_EXTERNAL, Shit::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
             Shit::PipelineStageFlagBits::BOTTOM_OF_PIPE_BIT, Shit::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT}};

        renderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(),
            attachmentDesc.data(),
            std::size(subpassDescs),
            subpassDescs,
            std::size(subpassDependencies),
            subpassDependencies,
        });
        Shit::ClearColorValueFloat clearColor = {};
        Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
        auto attachmentCount = renderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        renderTarget.clearValues.assign(attachmentCount, clearColor);
        renderTarget.clearValues[6] = clearDepthStencil;
    }
    void createRenderTarget() {
        // create framebuffer
        // color
        Shit::ImageCreateInfo colorImageCI{
            Shit::ImageCreateFlagBits::MUTABLE_FORMAT_BIT,  // create flags
            Shit::ImageType::TYPE_2D,
            Shit::Format::R16G16B16A16_SFLOAT,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,                     // mipmap levels,
            COLOR_ATTACHMENT_NUM,  // array layers
            _sampleCount,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::INPUT_ATTACHMENT_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        Shit::ImageViewCreateInfo colorImageViewCI{0,
                                                   Shit::ImageViewType::TYPE_2D,
                                                   Shit::Format::R16G16B16A16_SFLOAT,
                                                   {},
                                                   {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

        auto attachmentCount = renderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        auto depthFormat = renderTarget.renderPass->GetCreateInfoPtr()->pAttachments[attachmentCount - 1].format;
        //================
        Shit::ImageCreateInfo depthImageCI{
            {},  // create flags
            Shit::ImageType::TYPE_2D,
            depthFormat,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,  // mipmap levels,
            1,  // array layers
            _sampleCount,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::INPUT_ATTACHMENT_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

        Shit::ImageViewCreateInfo depthImageViewCI{
            0,
            Shit::ImageViewType::TYPE_2D,
            depthFormat,
            {},
            {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}};

        Shit::FramebufferCreateInfo frameBufferCI{
            renderTarget.renderPass,
            0,
            0,
            Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                           _swapchain->GetCreateInfoPtr()->imageExtent.height},
            1,
        };

        auto frameCount = _swapchain->GetImageCount();
        for (int i = 0; i < frameCount; ++i) {
            // deferred render target
            auto pImage = renderTarget.images.emplace_back(_device->Create(Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                Shit::Format::R8G8B8A8_UNORM,
                {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height,
                 1},
                1,  // mipmap levels,
                1,  // array layers
                _sampleCount,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT}));
            renderTarget.imageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{pImage,
                                                          Shit::ImageViewType::TYPE_2D,
                                                          pImage->GetCreateInfoPtr()->format,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}));

            // gpass images
            colorImageViewCI.pImage = renderTarget.images.emplace_back(_device->Create(colorImageCI));
            for (int j = 0; j < COLOR_ATTACHMENT_NUM; ++j) {
                colorImageViewCI.format = renderTarget.renderPass->GetCreateInfoPtr()->pAttachments[1 + j].format;
                colorImageViewCI.subresourceRange.baseArrayLayer = j;
                renderTarget.imageViews.emplace_back(_device->Create(colorImageViewCI));
            }
            depthImageViewCI.pImage = renderTarget.images.emplace_back(_device->Create(depthImageCI));
            renderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            frameBufferCI.attachmentCount = attachmentCount;
            frameBufferCI.pAttachments = &renderTarget.imageViews[i * attachmentCount];
            renderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
        }
    }
    void createDefaultRenderPass() override {
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // present color
            {_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference presentColorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpass{Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &presentColorAttachment};

        _defaultRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(),
            attachmentDesc.data(),
            1,
            &subpass,
        });
        _defaultRenderTarget.clearValues = {Shit::ClearColorValueFloat{}};
    }
    void createDefaultRenderTarget() override {
        auto attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        for (int i = 0; i < _swapchain->GetImageCount(); ++i) {
            // swapchain image
            _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            _defaultRenderTarget.frameBuffers.emplace_back(_device->Create(Shit::FramebufferCreateInfo{
                _defaultRenderTarget.renderPass,
                attachmentCount,
                &_defaultRenderTarget.imageViews[i * attachmentCount],
                Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                               _swapchain->GetCreateInfoPtr()->imageExtent.height},
                1,
            }));
        }
    }

    void preparePipelines() {
        writePipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, writeVertShaderPath, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                     static_cast<Shader *>(_shaderManager->create(
                                         buildShaderPath(_device, writeFragShaderPath, _rendererVersion),
                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        readPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, readVertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, readFragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        postProcessPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, readVertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(
                _shaderManager->create(buildShaderPath(_device, postProcessFragShaderPath, _rendererVersion),
                                       ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout const *> descriptorSetLayouts{
            _transformDescriptorSetLayout,
            _cameraDescriptorSetLayout,
            _materialDescriptorSetLayout,
        };

        writePipelineInfo.pipelineLayout = _device->Create(
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

        // pipelina layout
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::UNIFORM_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {1, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {2, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {3, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {4, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {5, Shit::DescriptorType::INPUT_ATTACHMENT, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {6, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };

        descriptorSetLayouts = {
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings) - 1, bindings}),
            _cameraDescriptorSetLayout,
            _lightDescriptorSetLayout,
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[6]}),
        };
        Shit::PushConstantRange pcRange{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0, 0, sizeof(UBORead)};
        readPipelineInfo.pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{
            (uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data(), 1, &pcRange});

        uboRead.sampleCount = (int)_sampleCount;
        std::vector<Shit::DescriptorSetLayout const *> tempSetLayouts(getFramesInFlight(), descriptorSetLayouts[0]);

        readPipelineInfo.descriptorSets.resize(tempSetLayouts.size());
        _descriptorPool->Allocate(
            Shit::DescriptorSetAllocateInfo{(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
            readPipelineInfo.descriptorSets.data());

        tempSetLayouts.assign(lights.size(), descriptorSetLayouts[3]);
        shadowDescriptorSets.resize(lights.size());
        _descriptorPool->Allocate(
            Shit::DescriptorSetAllocateInfo{(uint32_t)tempSetLayouts.size(), tempSetLayouts.data()},
            shadowDescriptorSets.data());

        // postprocess pipeline descriptor sets
        {
            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto descriptorSetLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});
            descriptorSetLayouts.assign(getFramesInFlight(), descriptorSetLayout);

            Shit::PushConstantRange pcRange{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(PostProcessPC)};

            postProcessPipelineInfo.pipelineLayout =
                _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout, 1, &pcRange});
            postProcessPipelineInfo.descriptorSets.resize(descriptorSetLayouts.size());

            _descriptorPool->Allocate({(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()},
                                      postProcessPipelineInfo.descriptorSets.data());
        }
    }
    void createWritePipeline() {
        //===============================
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : writePipelineInfo.shaders) {
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

        std::vector<Shit::PipelineColorBlendAttachmentState> colorBlendAttachmentStates(
            COLOR_ATTACHMENT_NUM, {false,
                                   {},
                                   {},
                                   {},
                                   {},
                                   {},
                                   {},
                                   Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
                                       Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT});

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        writePipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
                (uint32_t)colorBlendAttachmentStates.size(),
                colorBlendAttachmentStates.data(),
            },  // PipelineColorBlendStateCreateInfo
            {
                (uint32_t)std::size(dynamicStates),
                dynamicStates,
            },  // PipelineDynamicStateCreateInfo
            writePipelineInfo.pipelineLayout,
            renderTarget.renderPass,
            0});
    }
    void createReadPipeline() {
        //===============================
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : readPipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            true,
            Shit::BlendFactor::ONE,
            Shit::BlendFactor::ONE,
            Shit::BlendOp::ADD,
            Shit::BlendFactor::ONE,
            Shit::BlendFactor::ONE,
            Shit::BlendOp::ADD,
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
                Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT};

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        readPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            readPipelineInfo.pipelineLayout,
            renderTarget.renderPass,
            1});
    }
    void createPostProcessPipeline() {
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : postProcessPipelineInfo.shaders) {
            p->load();
            shaderStageCIs.emplace_back(p->getStage(), p->getHandle(), p->getEntryName().data());
        }
        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
            {},
            {},
            {},
            {},
            {},
            {},
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
                Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT};

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        postProcessPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            postProcessPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void updateDescriptorSets() {
        auto attachmentCount = renderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        std::vector<Shit::DescriptorImageInfo> imagesInfo(getFramesInFlight() * attachmentCount);

        auto sampler = getSampler("linear");

        std::vector<Shit::WriteDescriptorSet> writes;
        // write
        for (uint32_t i = 0; i < getFramesInFlight(); ++i) {
            imagesInfo[i * attachmentCount] = {sampler, renderTarget.imageViews[i * attachmentCount],
                                               Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

            writes.emplace_back(Shit::WriteDescriptorSet{postProcessPipelineInfo.descriptorSets[i], 0, 0, 1,
                                                         Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                         &imagesInfo[i * attachmentCount]});
            for (uint32_t j = 1; j < 6; ++j) {
                imagesInfo[i * attachmentCount + j] = Shit::DescriptorImageInfo{
                    0, renderTarget.imageViews[i * attachmentCount + j], Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

                writes.emplace_back(Shit::WriteDescriptorSet{readPipelineInfo.descriptorSets[i], j, 0, 1,
                                                             Shit::DescriptorType::INPUT_ATTACHMENT,
                                                             &imagesInfo[i * attachmentCount + j]});
            }
        }

        std::vector<Shit::DescriptorImageInfo> imagesInfo2;
        imagesInfo2.resize(lights.size());
        for (int i = 0; i < lights.size(); ++i) {
            imagesInfo2[i] = {sampler, shadowRenderTargets[i].imageViews[0],
                              Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            writes.emplace_back(Shit::WriteDescriptorSet{
                shadowDescriptorSets[i], 6, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo2[i]});
        }
        _device->UpdateDescriptorSets(writes);
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
        for (size_t j = 0; j < lights.size(); ++j) {
            auto &&e = shadowRenderTargets[j];
            cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{e.renderPass,
                                                                 e.frameBuffers[0],
                                                                 {{}, {SHADOW_MAP_WIDTH, SHADOW_MAP_WIDTH}},
                                                                 (uint32_t)e.clearValues.size(),
                                                                 e.clearValues.data(),
                                                                 Shit::SubpassContents::INLINE});

            // bind camera
            auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, shadowPipelineInfo.pipelineLayout, 1, 1, &descriptorSet});

            // bind light
            descriptorSet = lights[j]->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, shadowPipelineInfo.pipelineLayout, 3, 1, &descriptorSet});

            cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &shadow_viewport});
            cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &shadow_scissor});

            cmdBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, shadowPipelineInfo.pipeline});

            cmdBuffer->SetDepthBias({depthBiasContantFactor, depthBiasClamp, depthBiasSlopFactor});

            for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), shadowPipelineInfo.pipelineLayout);
            cmdBuffer->EndRenderPass();
        }

        // draw scene
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{renderTarget.renderPass,
                                                             renderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)renderTarget.clearValues.size(),
                                                             renderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        // bind camera
        auto descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, writePipelineInfo.pipelineLayout, 1, 1, &descriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, writePipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), writePipelineInfo.pipelineLayout);

        cmdBuffer->NextSubpass(Shit::SubpassContents::INLINE);
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipeline});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   readPipelineInfo.pipelineLayout, 0, 1,
                                                                   &readPipelineInfo.descriptorSets[_currentFrame]});

        // bind camera
        descriptorSet = _editorCamera->getDescriptorSet(imageIndex);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipelineLayout, 1, 1, &descriptorSet});

        cmdBuffer->PushConstants(
            {readPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0, sizeof(UBORead), &uboRead});
        if (uboRead.renderMode == 0) {
            // bind light
            for (int j = 0; j < lights.size(); ++j) {
                auto l = lights[j];
                descriptorSet = l->getDescriptorSet();
                cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                    Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipelineLayout, 2, 1, &descriptorSet});
                cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                           readPipelineInfo.pipelineLayout, 3, 1,
                                                                           &shadowDescriptorSets[j]});
                cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
            }
            static UBORead tempUBORead{7, (int)_sampleCount};
            cmdBuffer->PushConstants({readPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0,
                                      sizeof(UBORead), &tempUBORead});
            cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
        } else {
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipelineLayout, 3, 1, &shadowDescriptorSets[0]});
            descriptorSet = lights[0]->getDescriptorSet();
            cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::GRAPHICS, readPipelineInfo.pipelineLayout, 2, 1, &descriptorSet});

            cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
        }

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

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, postProcessPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, postProcessPipelineInfo.pipelineLayout, 0,
                                         1, &postProcessPipelineInfo.descriptorSets[_currentFrame]});
        cmdBuffer->PushConstants({postProcessPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0,
                                  sizeof(PostProcessPC), &postProcessPC});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("settings");

        static bool flag = false;
        static const char *items[]{
            "render", "pos", "albedo", "normal", "metallic", "roughness", "ao", "emission",
        };
        if (ImGui::BeginCombo("input attachment", items[uboRead.renderMode])) {
            for (int i = 0; i < std::size(items); ++i) {
                const bool is_selected = uboRead.renderMode == i;
                if (ImGui::Selectable(items[i], is_selected)) {
                    uboRead.renderMode = i;
                    _needUpdateDefaultCommandBuffer = true;
                }
            }
            ImGui::EndCombo();
        }
        _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("depthBiasContantFactor", &depthBiasContantFactor, 1.f);
        _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("depthBiasClamp", &depthBiasClamp, 1.f);
        _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("depthBiasSlopFactor", &depthBiasSlopFactor, 1.f);

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
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enable fxaa", (bool *)&postProcessPC.fxaa);
        _needUpdateDefaultCommandBuffer |= ImGui::Checkbox("highlightArea", (bool *)&postProcessPC.highlightArea);
        _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("fxaaThreashold", &postProcessPC.fxaaThreashold, 0.01);
        _needUpdateDefaultCommandBuffer |=
            ImGui::InputFloat("fxaaThreasholdMin", &postProcessPC.fxaaThreasholdMin, 0.01);
        _needUpdateDefaultCommandBuffer |= ImGui::InputFloat("fxaaSharpness", &postProcessPC.fxaaSharpness, 0.01);

        ImGui::End();
    }
    void recreateSwapchainListener() {
        for (auto &&e : renderTarget.imageViews) _device->Destroy(e);
        for (auto &&e : renderTarget.images) _device->Destroy(e);
        for (auto &&e : renderTarget.frameBuffers) _device->Destroy(e);
        renderTarget.images.clear();
        renderTarget.imageViews.clear();
        renderTarget.frameBuffers.clear();
        createRenderTarget();

        updateDescriptorSets();
    }
    void prepare() override {
        loadAssets();
        createShadowRenderTargets();
        createShadowPipeline();

        createRenderPass();
        createRenderTarget();

        preparePipelines();

        createWritePipeline();
        createReadPipeline();

        createPostProcessPipeline();

        updateDescriptorSets();

        addRecreateSwapchainListener(std::bind(&Hello::recreateSwapchainListener, this));
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_1)