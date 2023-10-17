#include "common/appbase.h"
#include "common/entry.h"
static auto writeVertShaderPath = "ao/write.vert";
static auto writeFragShaderPath = "ao/write.frag";
static auto readVertShaderPath = "ao/read.vert";
static auto readFragShaderPath = "ao/read.frag";
static auto aoCompShaderPath = "ao/ao.comp";
static auto shadingCompShaderPath = "ao/shading.comp";

static const char *testModelPaths[]{
    SHIT_SOURCE_DIR "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/plane.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/Sponza/Sponza.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/cube.obj",
    // SHIT_SOURCE_DIR "/assets/models/obj/sampleroom.obj",
};
#define COLOR_ATTACHMENT_NUM 5

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    Shit::PipelineLayout *pipelineLayout;
    std::vector<Shit::DescriptorSet *> descriptorSets;
};

struct AOPushConstant {
    int aoType;
    int sampleStep{16};
    int sampleStepNum{2};
    int sampleDirections{4};
    float maxRadius0{10};
    float maxRadius1{15};
};

struct ShadingPushConstant {
    int renderMode;
    float aoBlur;
};

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
    PipelineInfo gpassPipelineInfo;
    PipelineInfo aoPipelineInfo;
    PipelineInfo shadingPipelineInfo;
    PipelineInfo postProcessPipelineInfo;

    RenderTarget gpassRenderTarget;

    std::vector<Shit::Image *> aoImages;
    std::vector<Shit::ImageView *> aoImageViews;

    std::vector<Shit::Image *> shadingImages;
    std::vector<Shit::ImageView *> shadingImageViews;

    AOPushConstant aoPc{};
    ShadingPushConstant shadingPc{};
    bool enableBlur = true;

    GameObject *modelObject;
    GameObject *lightRoot;

    Shit::Sampler *depthSampler;
    Shit::Sampler *posSampler;

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
        modelObject->getComponent<Transform>()->scale({0.1, 0.1, 0.1});
        // for (auto it = modelSrcs[1]->meshBegin(), end = modelSrcs[1]->meshEnd();
        // it != end; ++it)
        //{
        //	auto child = modelObject->addChild();
        //	child->addComponent<MeshRenderer>(it->get());
        //	child->getComponent<Transform>()->scale({10, 1, 10})->translate({0, -1,
        // 0});
        // }

        depthSampler = createSampler("depthSampler", {Shit::Filter::LINEAR,
                                                      Shit::Filter::LINEAR,
                                                      Shit::SamplerMipmapMode::NEAREST,
                                                      Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                      Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                      Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                      0,
                                                      false,
                                                      0,
                                                      false,
                                                      {},
                                                      -1000,
                                                      1000,
                                                      Shit::BorderColor::FLOAT_OPAQUE_WHITE});
        posSampler = createSampler("posSampler", {Shit::Filter::LINEAR,
                                                  Shit::Filter::LINEAR,
                                                  Shit::SamplerMipmapMode::NEAREST,
                                                  Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                  Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                  Shit::SamplerWrapMode::CLAMP_TO_BORDER,
                                                  0,
                                                  false,
                                                  0,
                                                  false,
                                                  {},
                                                  -1000,
                                                  1000,
                                                  Shit::BorderColor::FLOAT_TRANSPARENT_BLACK});
    }
    void createGpassRenderPass() {
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // write geometry pass attachments
            // postion
            {Shit::Format::R16G16B16A16_SFLOAT, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // normal
            {Shit::Format::R16G16B16A16_SNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // diffuses
            {Shit::Format::R16G16B16A16_UNORM, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // specular shininess
            {Shit::Format::R16G16B16A16_SFLOAT, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
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
            {0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL}, {1, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            {2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL}, {3, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            {4, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference depthAttachment{5, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDescs[]{
            {Shit::PipelineBindPoint::GRAPHICS, 0, 0, (uint32_t)std::size(colorAttachments), colorAttachments, 0,
             &depthAttachment},
        };
        gpassRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(),
            attachmentDesc.data(),
            std::size(subpassDescs),
            subpassDescs,
        });
    }
    void createGpassFramebuffer() {
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
            Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        Shit::ImageViewCreateInfo colorImageViewCI{0,
                                                   Shit::ImageViewType::TYPE_2D,
                                                   Shit::Format::R16G16B16A16_SFLOAT,
                                                   {},
                                                   {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

        //================
        auto attachmentCount = gpassRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        auto depthFormat = gpassRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[attachmentCount - 1].format;
        Shit::ImageCreateInfo depthImageCI{
            {},  // create flags
            Shit::ImageType::TYPE_2D,
            depthFormat,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,  // mipmap levels,
            1,  // array layers
            _sampleCount,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

        Shit::ImageViewCreateInfo depthImageViewCI{
            0, Shit::ImageViewType::TYPE_2D, depthFormat, {}, {Shit::ImageAspectFlagBits::DEPTH_BIT, 0, 1, 0, 1}};

        Shit::FramebufferCreateInfo frameBufferCI{
            gpassRenderTarget.renderPass,
            0,
            0,
            Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                           _swapchain->GetCreateInfoPtr()->imageExtent.height},
            1,
        };

        for (int i = 0; i < _swapchain->GetImageCount(); ++i) {
            // gpass images
            colorImageViewCI.pImage = gpassRenderTarget.images.emplace_back(_device->Create(colorImageCI));
            for (int j = 0; j < COLOR_ATTACHMENT_NUM; ++j) {
                colorImageViewCI.format = gpassRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[j].format;
                colorImageViewCI.subresourceRange.baseArrayLayer = j;
                gpassRenderTarget.imageViews.emplace_back(_device->Create(colorImageViewCI));
            }
            depthImageViewCI.pImage = gpassRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            gpassRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            frameBufferCI.attachmentCount = attachmentCount;
            frameBufferCI.pAttachments = &gpassRenderTarget.imageViews[i * attachmentCount];
            gpassRenderTarget.frameBuffers.emplace_back(_device->Create(frameBufferCI));
        }
        gpassRenderTarget.clearValues.assign(attachmentCount, Shit::ClearColorValueFloat{});
        gpassRenderTarget.clearValues.back() = Shit::ClearDepthStencilValue{1., 0};
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
        {
            // gpass pipelina layout
            gpassPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                             buildShaderPath(_device, writeVertShaderPath, _rendererVersion),
                                             ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                         static_cast<Shader *>(_shaderManager->create(
                                             buildShaderPath(_device, writeFragShaderPath, _rendererVersion),
                                             ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

            Shit::DescriptorSetLayout const *descriptorSetLayouts[]{
                _transformDescriptorSetLayout,
                _cameraDescriptorSetLayout,
                _materialDescriptorSetLayout,
            };

            gpassPipelineInfo.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{(uint32_t)std::size(descriptorSetLayouts), descriptorSetLayouts});
        }
        {
            // ao pipeline
            aoPipelineInfo.shaders = {
                static_cast<Shader *>(
                    _shaderManager->create(buildShaderPath(_device, aoCompShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}})),
            };
            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {2, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {3, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            };
            auto descriptorSetLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

            auto count = getFramesInFlight();
            std::vector<Shit::DescriptorSetLayout const *> setLayouts(count, descriptorSetLayout);
            aoPipelineInfo.descriptorSets.resize(count);
            _descriptorPool->Allocate({count, setLayouts.data()}, aoPipelineInfo.descriptorSets.data());

            auto pcRange =
                Shit::PushConstantRange{Shit::ShaderStageFlagBits::COMPUTE_BIT, 2, 0, sizeof(AOPushConstant)};
            setLayouts = {descriptorSetLayout, _cameraDescriptorSetLayout};
            aoPipelineInfo.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{(uint32_t)setLayouts.size(), setLayouts.data(), 1, &pcRange});
        }
        {
            // shading pipeline layout
            shadingPipelineInfo.shaders = {
                static_cast<Shader *>(
                    _shaderManager->create(buildShaderPath(_device, shadingCompShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}})),
            };
            Shit::DescriptorSetLayoutBinding bindings[]{
                // set 1
                {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {2, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {3, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                {4, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                // set 2
                {5, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
                // set 3
                {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            };
            Shit::DescriptorSetLayout const *descriptorSetLayouts[]{
                _cameraDescriptorSetLayout,
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{5, &bindings[0]}),
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[5]}),
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[6]}),
            };

            auto count = getFramesInFlight();
            shadingPipelineInfo.descriptorSets.resize(count * 3);
            std::vector<Shit::DescriptorSetLayout const *> setLayouts(count, descriptorSetLayouts[1]);
            _descriptorPool->Allocate({count, setLayouts.data()}, &shadingPipelineInfo.descriptorSets[0]);
            setLayouts.assign(count, descriptorSetLayouts[2]);
            _descriptorPool->Allocate({count, setLayouts.data()}, &shadingPipelineInfo.descriptorSets[count]);
            setLayouts.assign(count, descriptorSetLayouts[3]);
            _descriptorPool->Allocate({count, setLayouts.data()}, &shadingPipelineInfo.descriptorSets[2 * count]);

            auto pcRange =
                Shit::PushConstantRange{Shit::ShaderStageFlagBits::COMPUTE_BIT, 0, 0, sizeof(ShadingPushConstant)};

            shadingPipelineInfo.pipelineLayout = _device->Create(
                Shit::PipelineLayoutCreateInfo{std::size(descriptorSetLayouts), descriptorSetLayouts, 1, &pcRange});
        }
        {
            // postprocess pipeline
            postProcessPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                                   buildShaderPath(_device, readVertShaderPath, _rendererVersion),
                                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                               static_cast<Shader *>(_shaderManager->create(
                                                   buildShaderPath(_device, readFragShaderPath, _rendererVersion),
                                                   ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

            Shit::DescriptorSetLayoutBinding bindings[]{
                {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            };
            auto descriptorSetLayout =
                _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

            auto count = getFramesInFlight();
            std::vector<Shit::DescriptorSetLayout const *> setLayouts(count, descriptorSetLayout);
            postProcessPipelineInfo.descriptorSets.resize(count);
            _descriptorPool->Allocate({count, setLayouts.data()}, postProcessPipelineInfo.descriptorSets.data());

            postProcessPipelineInfo.pipelineLayout =
                _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});
        }
    }
    void createGpassPipeline() {
        //===============================
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : gpassPipelineInfo.shaders) {
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

        gpassPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            gpassPipelineInfo.pipelineLayout,
            gpassRenderTarget.renderPass,
            0});
    }
    void createAoPipeline() {
        auto p = aoPipelineInfo.shaders[0];
        p->load();
        // uint32_t constantId = 0;
        // uint32_t constantValue = 1024;
        // Shit::SpecializationInfo spec{1, &constantId, &constantValue};
        auto shaderStageCI =
            Shit::PipelineShaderStageCreateInfo{p->getStage(), p->getHandle(), p->getEntryName().data()};

        auto count = getFramesInFlight();
        auto extent = Shit::Extent3D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                                     _swapchain->GetCreateInfoPtr()->imageExtent.height, 1};
        for (int i = 0; i < count; ++i) {
            auto pImage = aoImages.emplace_back(_device->Create(
                Shit::ImageCreateInfo{{},
                                      Shit::ImageType::TYPE_2D,
                                      Shit::Format::R16_SFLOAT,
                                      extent,
                                      0,
                                      1,
                                      Shit::SampleCountFlagBits::BIT_1,
                                      Shit::ImageTiling::OPTIMAL,
                                      Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                      Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                      Shit::ImageLayout::GENERAL}));
            aoImageViews.emplace_back(_device->Create(Shit::ImageViewCreateInfo{
                pImage,
                Shit::ImageViewType::TYPE_2D,
                Shit::Format::R16_SFLOAT,
                {},
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, pImage->GetCreateInfoPtr()->mipLevels, 0, 1}}));
        }
        aoPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, aoPipelineInfo.pipelineLayout});
    }
    void createShadingPipeline() {
        auto p = shadingPipelineInfo.shaders[0];
        p->load();
        auto shaderStageCI =
            Shit::PipelineShaderStageCreateInfo{p->getStage(), p->getHandle(), p->getEntryName().data()};

        auto count = getFramesInFlight();
        auto extent = Shit::Extent3D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                                     _swapchain->GetCreateInfoPtr()->imageExtent.height, 1};
        for (int i = 0; i < count; ++i) {
            auto pImage = shadingImages.emplace_back(_device->Create(
                Shit::ImageCreateInfo{{},
                                      Shit::ImageType::TYPE_2D,
                                      Shit::Format::R8G8B8A8_UNORM,
                                      extent,
                                      1,
                                      1,
                                      Shit::SampleCountFlagBits::BIT_1,
                                      Shit::ImageTiling::OPTIMAL,
                                      Shit::ImageUsageFlagBits::STORAGE_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                                      Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                      Shit::ImageLayout::GENERAL}));
            shadingImageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{pImage,
                                                          Shit::ImageViewType::TYPE_2D,
                                                          Shit::Format::R8G8B8A8_UNORM,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}}));
        }

        shadingPipelineInfo.pipeline =
            _device->Create(Shit::ComputePipelineCreateInfo{shaderStageCI, shadingPipelineInfo.pipelineLayout});
    }
    void createPostProcessPipeline() {
        //===============================
        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : postProcessPipelineInfo.shaders) {
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
        auto sampler = getSampler("linear");

        auto frameCount = getFramesInFlight();
        auto gpassAttachmentCount = gpassRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        std::vector<Shit::DescriptorImageInfo> imagesInfo;
        std::vector<Shit::WriteDescriptorSet> writes;
        {
            // update ao descriptor set
            auto count = 4;
            imagesInfo.resize(frameCount * count);
            for (uint32_t i = 0; i < frameCount; ++i) {
                imagesInfo[i * count + 0] = {depthSampler,
                                             gpassRenderTarget.imageViews[(i + 1) * gpassAttachmentCount - 1],
                                             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
                imagesInfo[i * count + 1] = {posSampler, gpassRenderTarget.imageViews[i * gpassAttachmentCount + 0],
                                             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
                imagesInfo[i * count + 2] = {sampler, gpassRenderTarget.imageViews[i * gpassAttachmentCount + 1],
                                             Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
                imagesInfo[i * count + 3] = {0, aoImageViews[i], Shit::ImageLayout::GENERAL};
                writes.emplace_back(aoPipelineInfo.descriptorSets[i], 0, 0, 1,
                                    Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[i * count + 0]);
                writes.emplace_back(aoPipelineInfo.descriptorSets[i], 1, 0, 1,
                                    Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[i * count + 1]);
                writes.emplace_back(aoPipelineInfo.descriptorSets[i], 2, 0, 1,
                                    Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[i * count + 2]);
                writes.emplace_back(aoPipelineInfo.descriptorSets[i], 3, 0, 1, Shit::DescriptorType::STORAGE_IMAGE,
                                    &imagesInfo[i * count + 3]);
            }
            _device->UpdateDescriptorSets(writes);
            writes.clear();
        }
        {
            // update shading descriptor set
            auto count = 7;
            imagesInfo.resize(frameCount * count);
            for (uint32_t i = 0; i < frameCount; ++i) {
                for (uint32_t j = 0; j < 5; ++j) {
                    imagesInfo[i * count + j] =
                        Shit::DescriptorImageInfo{sampler, gpassRenderTarget.imageViews[i * gpassAttachmentCount + j],
                                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};

                    writes.emplace_back(Shit::WriteDescriptorSet{shadingPipelineInfo.descriptorSets[i], j, 0, 1,
                                                                 Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                                 &imagesInfo[i * count + j]});
                }
                imagesInfo[i * count + 5] = {sampler, aoImageViews[i], Shit::ImageLayout::GENERAL};
                imagesInfo[i * count + 6] = {0, shadingImageViews[i], Shit::ImageLayout::GENERAL};
                writes.emplace_back(shadingPipelineInfo.descriptorSets[i + frameCount], 5, 0, 1,
                                    Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[i * count + 5]);
                writes.emplace_back(shadingPipelineInfo.descriptorSets[i + 2 * frameCount], 0, 0, 1,
                                    Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[i * count + 6]);
            }
            _device->UpdateDescriptorSets(writes);
            writes.clear();
        }
        {
            // update postprocess descriptor set
            imagesInfo.resize(frameCount);
            for (uint32_t i = 0; i < frameCount; ++i) {
                imagesInfo[i] = {sampler, shadingImageViews[i], Shit::ImageLayout::GENERAL};
                writes.emplace_back(postProcessPipelineInfo.descriptorSets[i], 0, 0, 1,
                                    Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[i]);
            }
            _device->UpdateDescriptorSets(writes);
            writes.clear();
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

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];

        // bind camera
        auto cameraDescriptorSet = _editorCamera->getDescriptorSet(imageIndex);

        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});

        // gpass
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{gpassRenderTarget.renderPass,
                                                             gpassRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)gpassRenderTarget.clearValues.size(),
                                                             gpassRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, gpassPipelineInfo.pipelineLayout, 1, 1, &cameraDescriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, gpassPipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), gpassPipelineInfo.pipelineLayout);

        cmdBuffer->EndRenderPass();

        // ao pipeline
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, aoPipelineInfo.pipeline});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE, aoPipelineInfo.pipelineLayout, 1, 1, &cameraDescriptorSet});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE,
                                                                   aoPipelineInfo.pipelineLayout, 0, 1,
                                                                   &aoPipelineInfo.descriptorSets[_currentFrame]});

        cmdBuffer->PushConstants(
            {aoPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::COMPUTE_BIT, 0, sizeof(AOPushConstant), &aoPc});

        cmdBuffer->Dispatch({ext.width * ext.height / 1024 + 1, 1, 1});

        cmdBuffer->GenerateMipmap(
            {aoImages[_currentFrame], Shit::Filter::LINEAR, Shit::ImageLayout::GENERAL, Shit::ImageLayout::GENERAL});

        // shading pipeline
        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, shadingPipelineInfo.pipeline});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE, shadingPipelineInfo.pipelineLayout, 0, 1, &cameraDescriptorSet});

        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE,
                                                                   shadingPipelineInfo.pipelineLayout, 1, 1,
                                                                   &shadingPipelineInfo.descriptorSets[_currentFrame]});

        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE, shadingPipelineInfo.pipelineLayout, 2, 1,
                                         &shadingPipelineInfo.descriptorSets[_currentFrame + getFramesInFlight()]});

        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::COMPUTE, shadingPipelineInfo.pipelineLayout, 3, 1,
                                         &shadingPipelineInfo.descriptorSets[_currentFrame + getFramesInFlight() * 2]});

        cmdBuffer->PushConstants({shadingPipelineInfo.pipelineLayout, Shit::ShaderStageFlagBits::COMPUTE_BIT, 0,
                                  sizeof(ShadingPushConstant), &shadingPc});

        cmdBuffer->Dispatch({ext.width * ext.height / 1024 + 1, 1, 1});

        Shit::ImageMemoryBarrier barrier{Shit::AccessFlagBits::SHADER_WRITE_BIT,
                                         Shit::AccessFlagBits::SHADER_READ_BIT,
                                         Shit::ImageLayout::GENERAL,
                                         Shit::ImageLayout::GENERAL,
                                         ST_QUEUE_FAMILY_IGNORED,
                                         ST_QUEUE_FAMILY_IGNORED,
                                         aoImages[_currentFrame],
                                         {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

        cmdBuffer->PipelineBarrier({Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                    Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                    {},
                                    0,
                                    0,
                                    0,
                                    0,
                                    1,
                                    &barrier});
        // post process pipeline
        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, postProcessPipelineInfo.pipeline});

        cmdBuffer->BindDescriptorSets(
            Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS, postProcessPipelineInfo.pipelineLayout, 0,
                                         1, &postProcessPipelineInfo.descriptorSets[_currentFrame]});

        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void setupImGui() {
        ImGui::Begin("settings");
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
            static const char *items[]{
                "GTAOUniform",
                "GTAOCosine",
            };
            if (ImGui::BeginCombo("ao mode", items[aoPc.aoType])) {
                for (int i = 0; i < std::size(items); ++i) {
                    const bool is_selected = (aoPc.aoType == i);
                    if (ImGui::Selectable(items[i], is_selected)) {
                        aoPc.aoType = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
        }
        _needUpdateDefaultCommandBuffer |= ImGui::InputInt("sampleStep", &aoPc.sampleStep);
        _needUpdateDefaultCommandBuffer |= ImGui::InputInt("sampleStepNum", &aoPc.sampleStepNum);
        _needUpdateDefaultCommandBuffer |= ImGui::InputInt("sampleDirections", &aoPc.sampleDirections);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("maxRadius0", &aoPc.maxRadius0, 0, aoPc.maxRadius1);
        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("maxRadius1", &aoPc.maxRadius1, aoPc.maxRadius0, 100);

        _needUpdateDefaultCommandBuffer |= ImGui::SliderFloat("aoblur", &shadingPc.aoBlur, 0.f, 10.f);

        //_needUpdateDefaultCommandBuffer |= ImGui::Checkbox("enableBlur",
        //&enableBlur);
        {
            static const char *items[]{
                "calc ao", "calc ao width multibounce", "render with ao", "texture ao", "no ao",
            };
            if (ImGui::BeginCombo("render mode", items[shadingPc.renderMode])) {
                for (int i = 0; i < std::size(items); ++i) {
                    const bool is_selected = (shadingPc.renderMode == i);
                    if (ImGui::Selectable(items[i], is_selected)) {
                        shadingPc.renderMode = i;
                        _needUpdateDefaultCommandBuffer = true;
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::End();
    }
    void recreateGpass() {
        for (auto e : gpassRenderTarget.images) _device->Destroy(e);
        gpassRenderTarget.images.clear();
        for (auto e : gpassRenderTarget.imageViews) _device->Destroy(e);
        gpassRenderTarget.imageViews.clear();
        for (auto e : gpassRenderTarget.frameBuffers) _device->Destroy(e);
        gpassRenderTarget.frameBuffers.clear();

        _device->Destroy(gpassPipelineInfo.pipeline);
        createGpassFramebuffer();
        updateDescriptorSets();
    }
    void prepare() override {
        loadAssets();
        createGpassRenderPass();
        createGpassFramebuffer();

        preparePipelines();
        createGpassPipeline();
        createAoPipeline();
        createShadingPipeline();
        createPostProcessPipeline();

        updateDescriptorSets();
        addRecreateSwapchainListener(std::bind(&Hello::recreateGpass, this));
    }
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_1)