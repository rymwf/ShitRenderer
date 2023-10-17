#include "common/appbase.h"
#include "common/entry.h"
static auto multiviewVertShaderPath = "multiview/multiview.vert";
static auto multiviewFragShaderPath = "multiview/multiview.frag";

static auto presentVertShaderPath = "multiview/present.vert";
static auto presentFragShaderPath = "multiview/present.frag";

static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/viking_room/viking_room.obj";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    std::vector<Shit::DescriptorSet *> descriptorSets;
};

class Hello : public AppBase {
    RenderTarget multiviewRenderTarget;
    PipelineInfo multiviewPipelineInfo;
    PipelineInfo presentPipelineInfo;

    GameObject *modelObject;
    Light *light;

    Camera *camera;
    Shit::Buffer *uboBuffer;

public:
    void createDefaultRenderPass() override {
        _defaultRenderTarget.clearValues = {
            Shit::ClearColorValueFloat{0.2, 0.2, 0.2, 1.},
            Shit::ClearDepthStencilValue{1., 0},
        };

        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // color
            {_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL},
            // depth
            {Shit::Format::D24_UNORM_S8_UINT, Shit::SampleCountFlagBits::BIT_1, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL},
        };
        Shit::AttachmentReference colorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        Shit::AttachmentReference depthAttachment{1, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDesc[]{
            Shit::PipelineBindPoint::GRAPHICS, 0, 0, 1, &colorAttachment, 0, &depthAttachment};

        _defaultRenderTarget.renderPass = _device->Create(
            Shit::RenderPassCreateInfo{(uint32_t)attachmentDesc.size(), attachmentDesc.data(), 1, subpassDesc});
        Shit::ClearColorValueFloat clearColor = {0.2, 0.2, 0.2, 1};
        Shit::ClearDepthStencilValue clearDepthStencil = {1., 0};
        _defaultRenderTarget.clearValues = {clearColor, clearDepthStencil};
    }
    void createDefaultRenderTarget() override {
        // create framebuffer
        // color
        Shit::ImageCreateInfo colorImageCI{
            {},  // create flags
            Shit::ImageType::TYPE_2D,
            _swapchain->GetCreateInfoPtr()->format,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,  // mipmap levels,
            1,  // array layers
            Shit::SampleCountFlagBits::BIT_1,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::TRANSFER_SRC_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};
        Shit::ImageViewCreateInfo colorImageViewCI{0,
                                                   Shit::ImageViewType::TYPE_2D,
                                                   _swapchain->GetCreateInfoPtr()->format,
                                                   {},
                                                   {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 1}};

        //================
        Shit::ImageCreateInfo depthImageCI{
            {},  // create flags
            Shit::ImageType::TYPE_2D,
            _defaultRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[1].format,
            {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
            1,  // mipmap levels,
            1,  // array layers
            Shit::SampleCountFlagBits::BIT_1,
            Shit::ImageTiling::OPTIMAL,
            Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
            Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT};

        Shit::ImageViewCreateInfo depthImageViewCI{
            0,
            Shit::ImageViewType::TYPE_2D,
            _defaultRenderTarget.renderPass->GetCreateInfoPtr()->pAttachments[1].format,
            {},
            {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 1}};

        Shit::FramebufferCreateInfo frameBufferCI{
            _defaultRenderTarget.renderPass,
            0,
            0,
            Shit::Extent2D{_swapchain->GetCreateInfoPtr()->imageExtent.width,
                           _swapchain->GetCreateInfoPtr()->imageExtent.height},
            1,
        };

        auto imageCount = _swapchain->GetImageCount();
        _defaultRenderTarget.frameBuffers.resize(imageCount);
        for (int i = 0; i < imageCount; ++i) {
            _defaultRenderTarget.images.emplace_back(_swapchain->GetImageByIndex(i));
            _defaultRenderTarget.imageViews.emplace_back(_swapchain->GetImageViewByIndex(i));

            depthImageViewCI.pImage = _defaultRenderTarget.images.emplace_back(_device->Create(depthImageCI));
            _defaultRenderTarget.imageViews.emplace_back(_device->Create(depthImageViewCI));

            frameBufferCI.attachmentCount = _defaultRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
            frameBufferCI.pAttachments = &_defaultRenderTarget.imageViews[i * frameBufferCI.attachmentCount];
            _defaultRenderTarget.frameBuffers[i] = _device->Create(frameBufferCI);
        }
    }
    void createMultiviewRenderTarget() {
        multiviewRenderTarget.clearValues = {
            Shit::ClearColorValueFloat{0.2, 0.2, 0.2, 1.},
            Shit::ClearDepthStencilValue{1., 0},
        };
        //
        Shit::Format depthFormatCandidates[]{Shit::Format::D24_UNORM_S8_UINT, Shit::Format::D32_SFLOAT_S8_UINT};
        auto depthFormat = _device->GetSuitableImageFormat(depthFormatCandidates, Shit::ImageTiling::OPTIMAL,
                                                           Shit::FormatFeatureFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT);

        // create renderpass
        std::vector<Shit::AttachmentDescription> attachmentDesc{
            // color
            {_swapchain->GetCreateInfoPtr()->format, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            // depth
            {Shit::Format::D24_UNORM_S8_UINT, _sampleCount, Shit::AttachmentLoadOp::CLEAR,
             Shit::AttachmentStoreOp::STORE, Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
             Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
            attachmentDesc[0].finalLayout = Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
            // resolve attachment
            attachmentDesc.emplace_back(_swapchain->GetCreateInfoPtr()->format, Shit::SampleCountFlagBits::BIT_1,
                                        Shit::AttachmentLoadOp::CLEAR, Shit::AttachmentStoreOp::STORE,
                                        Shit::AttachmentLoadOp::DONT_CARE, Shit::AttachmentStoreOp::DONT_CARE,
                                        Shit::ImageLayout::UNDEFINED, Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
            multiviewRenderTarget.clearValues.emplace_back(Shit::ClearColorValueFloat{0.2, 0.2, 0.2, 1.});
        }
        Shit::AttachmentReference colorAttachment{0, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};
        Shit::AttachmentReference depthAttachment{1, Shit::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        Shit::AttachmentReference resolveAttachment{2, Shit::ImageLayout::COLOR_ATTACHMENT_OPTIMAL};

        Shit::SubpassDescription subpassDesc[]{
            Shit::PipelineBindPoint::GRAPHICS,
            0,
            0,
            1,
            &colorAttachment,
            _sampleCount == Shit::SampleCountFlagBits::BIT_1 ? 0 : &resolveAttachment,
            &depthAttachment};

        uint32_t viewMask = 0b11;
        uint32_t correlationMask = 0b11;
        Shit::RenderPassMultiViewCreateInfo multiviewCI{1, &viewMask, 0, 0, 1, &correlationMask};

        multiviewRenderTarget.renderPass = _device->Create(Shit::RenderPassCreateInfo{
            (uint32_t)attachmentDesc.size(), attachmentDesc.data(), 1, subpassDesc, 0, 0, &multiviewCI});

        auto count = _swapchain->GetImageCount();
        multiviewRenderTarget.frameBuffers.resize(count);
        for (int i = 0; i < count; ++i) {
            auto pImage = multiviewRenderTarget.images.emplace_back(_device->Create(Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                _swapchain->GetCreateInfoPtr()->format,
                {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height,
                 1},
                1,
                2,
                _sampleCount,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            }));
            multiviewRenderTarget.imageViews.emplace_back(
                _device->Create(Shit::ImageViewCreateInfo{pImage,
                                                          Shit::ImageViewType::TYPE_2D_ARRAY,
                                                          pImage->GetCreateInfoPtr()->format,
                                                          {},
                                                          {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 2}}));
            //
            pImage = multiviewRenderTarget.images.emplace_back(_device->Create(Shit::ImageCreateInfo{
                {},
                Shit::ImageType::TYPE_2D,
                depthFormat,
                {_swapchain->GetCreateInfoPtr()->imageExtent.width, _swapchain->GetCreateInfoPtr()->imageExtent.height,
                 1},
                1,
                2,
                _sampleCount,
                Shit::ImageTiling::OPTIMAL,
                Shit::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
            }));
            multiviewRenderTarget.imageViews.emplace_back(_device->Create(Shit::ImageViewCreateInfo{
                pImage,
                Shit::ImageViewType::TYPE_2D_ARRAY,
                pImage->GetCreateInfoPtr()->format,
                {},
                {Shit::ImageAspectFlagBits::DEPTH_BIT | Shit::ImageAspectFlagBits::STENCIL_BIT, 0, 1, 0, 2}}));

            if (_sampleCount != Shit::SampleCountFlagBits::BIT_1) {
                // resolve
                pImage = multiviewRenderTarget.images.emplace_back(_device->Create(Shit::ImageCreateInfo{
                    {},
                    Shit::ImageType::TYPE_2D,
                    _swapchain->GetCreateInfoPtr()->format,
                    {_swapchain->GetCreateInfoPtr()->imageExtent.width,
                     _swapchain->GetCreateInfoPtr()->imageExtent.height, 1},
                    1,
                    2,
                    Shit::SampleCountFlagBits::BIT_1,
                    Shit::ImageTiling::OPTIMAL,
                    Shit::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT,
                    Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                }));
                multiviewRenderTarget.imageViews.emplace_back(
                    _device->Create(Shit::ImageViewCreateInfo{pImage,
                                                              Shit::ImageViewType::TYPE_2D_ARRAY,
                                                              pImage->GetCreateInfoPtr()->format,
                                                              {},
                                                              {Shit::ImageAspectFlagBits::COLOR_BIT, 0, 1, 0, 2}}));
            }

            // create framebuffers
            multiviewRenderTarget.frameBuffers[i] = _device->Create(Shit::FramebufferCreateInfo{
                multiviewRenderTarget.renderPass,
                (uint32_t)attachmentDesc.size(),
                &multiviewRenderTarget.imageViews[i * attachmentDesc.size()],
                _swapchain->GetCreateInfoPtr()->imageExtent,
                1,
            });
        }
    }

    void createMultiviewPipeline() {
        // pipelina layout
        Shit::DescriptorSetLayoutBinding descriptorSetLayoutBindings[]{
            {2, Shit::DescriptorType::UNIFORM_BUFFER, 1,
             Shit::ShaderStageFlagBits::VERTEX_BIT | Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        Shit::DescriptorSetLayoutCreateInfo descriptorSetLayoutCIs[]{
            {1, &descriptorSetLayoutBindings[0]},
        };
        Shit::DescriptorSetLayout *descriptorSetLayouts[]{
            _transformDescriptorSetLayout,
            _device->Create(descriptorSetLayoutCIs[0]),
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };
        multiviewPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayouts[1]}, multiviewPipelineInfo.descriptorSets.data());

        auto pipelineLayout =
            _device->Create(Shit::PipelineLayoutCreateInfo{std::size(descriptorSetLayouts), descriptorSetLayouts});

        //===============================
        multiviewPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                             buildShaderPath(_device, multiviewVertShaderPath, _rendererVersion),
                                             ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                         static_cast<Shader *>(_shaderManager->create(
                                             buildShaderPath(_device, multiviewFragShaderPath, _rendererVersion),
                                             ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : multiviewPipelineInfo.shaders) {
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

        multiviewPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            multiviewRenderTarget.renderPass,
            0});
    }
    void createPresentPipeline() {
        // pipelina layout
        Shit::DescriptorSetLayoutBinding descriptorSetLayoutBindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };

        auto count = _swapchain->GetImageCount();
        auto descriptorSetLayout = _device->Create(
            Shit::DescriptorSetLayoutCreateInfo{std::size(descriptorSetLayoutBindings), descriptorSetLayoutBindings});
        std::vector<Shit::DescriptorSetLayout *> setLayouts(count, descriptorSetLayout);
        presentPipelineInfo.descriptorSets.resize(count);
        _descriptorPool->Allocate({(uint32_t)setLayouts.size(), setLayouts.data()},
                                  presentPipelineInfo.descriptorSets.data());

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        //===============================
        presentPipelineInfo.shaders = {static_cast<Shader *>(_shaderManager->create(
                                           buildShaderPath(_device, presentVertShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
                                       static_cast<Shader *>(_shaderManager->create(
                                           buildShaderPath(_device, presentFragShaderPath, _rendererVersion),
                                           ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        std::vector<Shit::PipelineShaderStageCreateInfo> shaderStageCIs;
        for (auto p : presentPipelineInfo.shaders) {
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

        presentPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void preparePipelines() {
        createMultiviewRenderTarget();
        createMultiviewPipeline();
        createPresentPipeline();
    }

    void createCamera() {
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        auto cameraNode = _editorCamera->getParent()->addChild();
        camera = cameraNode->addComponent<Camera>(
            PerspectiveDescription{glm::radians(60.f), float(ext.width) / ext.height}, 0.1, 500.f);
        camera->getParentTransform()->translate({0.1, 0, 0})->yaw(glm::radians(10.f));

        camera->updateSignal.Connect(std::bind(&Hello::updateCamera, this, std::placeholders::_1));

        //============
        uboBuffer = _device->Create(Shit::BufferCreateInfo{
            {},
            sizeof(float) * 72,
            Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT});

        Shit::DescriptorBufferInfo bufferInfo{uboBuffer, 0, sizeof(float) * 72};
        Shit::WriteDescriptorSet writeSet{multiviewPipelineInfo.descriptorSets[0], 2, 0,          1,
                                          Shit::DescriptorType::UNIFORM_BUFFER,    0, &bufferInfo};
        _device->UpdateDescriptorSets({&writeSet, 1});
    }

    struct UBOCamera {
        glm::mat4 V;
        glm::mat4 P;
        glm::vec3 eyePos;
        int pad;
    };
    void updateCamera(Component const *) {
        UBOCamera a[]{
            {_editorCamera->getViewMatrix(), _editorCamera->getProjectionMatrix(),
             _editorCamera->getParentTransform()->getGlobalPosition()},
            {camera->getViewMatrix(), camera->getProjectionMatrix(), camera->getParentTransform()->getGlobalPosition()},
        };
        void *data;
        uboBuffer->MapMemory(0, sizeof(float) * 72, &data);
        memcpy(data, a, sizeof(float) * 72);
        uboBuffer->UnMapMemory();
    }

    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();
        modelObject->getComponent<Transform>()->pitch(glm::radians(-90.f))->roll(glm::radians(-90.f));

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
        }
        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-45.f))->yaw(45.f);
    }
    void writePresentDescriptors() {
        auto count = _swapchain->GetImageCount();
        std::vector<Shit::DescriptorImageInfo> imageInfos(count);
        std::vector<Shit::WriteDescriptorSet> writes(count);
        auto attachmentCount = multiviewRenderTarget.renderPass->GetCreateInfoPtr()->attachmentCount;
        for (int i = 0; i < count; ++i) {
            if (_sampleCount == Shit::SampleCountFlagBits::BIT_1) {
                imageInfos[i] = {getSampler("linear"), multiviewRenderTarget.imageViews[i * attachmentCount],
                                 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            } else {
                imageInfos[i] = {getSampler("linear"), multiviewRenderTarget.imageViews[i * attachmentCount + 2],
                                 Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL};
            }
            writes[i] = {presentPipelineInfo.descriptorSets[i],
                         0,
                         0,
                         1,
                         Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
                         &imageInfos[i]};
        }
        _device->UpdateDescriptorSets(writes);
    }

    void prepare() override {
        loadAssets();
        preparePipelines();
        writePresentDescriptors();
        createCamera();
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
        auto ext0 = multiviewRenderTarget.frameBuffers[0]->GetCreateInfoPtr()->extent;
        Shit::Viewport viewport0{0, 0, ext0.width, ext0.height, 0, 1};
        Shit::Rect2D scissor0{{}, ext0};

        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{}, ext};

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});
        cmdBuffer->BeginRenderPass(
            Shit::BeginRenderPassInfo{multiviewRenderTarget.renderPass,
                                      multiviewRenderTarget.frameBuffers[imageIndex],
                                      {{}, multiviewRenderTarget.frameBuffers[imageIndex]->GetCreateInfoPtr()->extent},
                                      (uint32_t)multiviewRenderTarget.clearValues.size(),
                                      multiviewRenderTarget.clearValues.data(),
                                      Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport0});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor0});

        // bind camera
        auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(multiviewPipelineInfo.pipeline);
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline->GetCreateInfoPtr()->pLayout, 1, 1,
                                                                   &multiviewPipelineInfo.descriptorSets[0]});

        // bind light
        auto lightDescriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &lightDescriptorSet});

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, multiviewPipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);

        cmdBuffer->EndRenderPass();

        // present
        cmdBuffer->BeginRenderPass(
            Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                      _defaultRenderTarget.frameBuffers[imageIndex],
                                      {{}, _defaultRenderTarget.frameBuffers[imageIndex]->GetCreateInfoPtr()->extent},
                                      (uint32_t)_defaultRenderTarget.clearValues.size(),
                                      _defaultRenderTarget.clearValues.data(),
                                      Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, presentPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS,
            dynamic_cast<Shit::GraphicsPipeline *>(presentPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout,
            0,
            1,
            &presentPipelineInfo.descriptorSets[_currentFrame],
        });
        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});
        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }

    void setupImGui() override {}
};
#include "common/entry.h"
EXAMPLE_MAIN2(Hello)