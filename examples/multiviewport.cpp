#include "common/appbase.h"
#include "common/entry.h"
static auto vertShaderPath = "multiviewport/multiviewport.vert";
static auto geomShaderPath = "multiviewport/multiviewport.geom";
static auto fragShaderPath = "multiviewport/multiviewport.frag";
// static auto testModelPath = SHIT_SOURCE_DIR
// "/assets/models/obj/sampleroom.obj";
static auto testModelPath = SHIT_SOURCE_DIR "/assets/models/obj/DamagedHelmet/DamagedHelmet.obj";

struct PipelineInfo {
    std::vector<Shader *> shaders;
    Shit::Pipeline *pipeline;
    std::vector<Shit::DescriptorSet *> descriptorSets;
};

class Hello : public AppBase {
    PipelineInfo pipelineInfo;
    GameObject *modelObject;
    Light *light;
    Camera *camera;

    Shit::Buffer *cameraUboBuffer;

public:
    void preparePipeline() {
        Shit::DescriptorSetLayoutBinding binding{2, Shit::DescriptorType::UNIFORM_BUFFER, 1,
                                                 Shit::ShaderStageFlagBits::VERTEX_BIT |
                                                     Shit::ShaderStageFlagBits::FRAGMENT_BIT |
                                                     Shit::ShaderStageFlagBits::GEOMETRY_BIT};
        pipelineInfo.descriptorSets.resize(1);
        auto setLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &binding});
        _descriptorPool->Allocate({1, &setLayout}, pipelineInfo.descriptorSets.data());

        Shit::DescriptorSetLayout *descriptorSetLayouts[]{
            _transformDescriptorSetLayout,
            setLayout,
            _materialDescriptorSetLayout,
            _lightDescriptorSetLayout,
        };

        auto pipelineLayout =
            _device->Create(Shit::PipelineLayoutCreateInfo{std::size(descriptorSetLayouts), descriptorSetLayouts});

        //===============================
        pipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, geomShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "GEOM"}})),
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
        Shit::Viewport viewports[2]{};
        Shit::Rect2D scissors[2]{};

        pipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
            (uint32_t)shaderStageCIs.size(),
            shaderStageCIs.data(),
            vertexInputStageCI,
            {Shit::PrimitiveTopology::TRIANGLE_LIST},  // PipelineInputAssemblyStateCreateInfo
            {2, viewports, 2, scissors},               // PipelineViewportStateCreateInfo
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
    void loadAssets() {
        auto model = static_cast<Model *>(_modelManager->create(testModelPath));
        model->load();

        modelObject = _gameObjects.emplace_back(std::make_unique<GameObject>()).get();

        for (auto it = model->meshBegin(), end = model->meshEnd(); it != end; ++it) {
            auto child = modelObject->addChild();
            child->addComponent<MeshRenderer>(it->get());
        }
        // add directional light
        light = _gameObjects.emplace_back(std::make_unique<GameObject>())->addComponent<Light>();
        light->getParentTransform()->pitch(glm::radians(-45.f))->yaw(45.f);
    }
    void createCamera() {
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        _editorCamera->setPerspective(glm::radians(60.f), float(ext.width / 2.) / ext.height);

        auto cameraNode = _editorCamera->getParent()->addChild();
        camera = cameraNode->addComponent<Camera>(
            PerspectiveDescription{glm::radians(60.f), float(ext.width / 2.) / ext.height}, 0.1, 500.f);
        camera->getParentTransform()->translate({0.1, 0, 0});
        camera->getParentTransform()->yaw(glm::radians(10.f));

        camera->updateSignal.Connect(std::bind(&Hello::updateCamera, this, std::placeholders::_1));

        //============
        cameraUboBuffer = _device->Create(Shit::BufferCreateInfo{
            {},
            sizeof(float) * 72,
            Shit::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Shit::BufferUsageFlagBits::TRANSFER_DST_BIT,
            Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT | Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT});

        Shit::DescriptorBufferInfo bufferInfo{cameraUboBuffer, 0, sizeof(float) * 72};
        Shit::WriteDescriptorSet writeSet{pipelineInfo.descriptorSets[0],       2, 0,          1,
                                          Shit::DescriptorType::UNIFORM_BUFFER, 0, &bufferInfo};
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
        cameraUboBuffer->MapMemory(0, sizeof(float) * 72, &data);
        memcpy(data, a, sizeof(float) * 72);
        cameraUboBuffer->UnMapMemory();
    }
    void prepare() override {
        loadAssets();
        preparePipeline();
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
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewports[]{
            {0, 0, ext.width / 2, ext.height, 0, 1},
            {ext.width / 2, 0, ext.width / 2, ext.height, 0, 1},
        };
        Shit::Rect2D scissors[]{
            {{}, {ext.width / 2, ext.height}},
            {{ext.width / 2, 0}, {ext.width / 2, ext.height}},
        };

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});

        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, std::size(viewports), viewports});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, std::size(scissors), scissors});

        auto pipeline = dynamic_cast<Shit::GraphicsPipeline *>(pipelineInfo.pipeline);

        // bind camera
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{Shit::PipelineBindPoint::GRAPHICS,
                                                                   pipeline->GetCreateInfoPtr()->pLayout, 1, 1,
                                                                   &pipelineInfo.descriptorSets[0]});

        // bind light
        auto lightDescriptorSet = light->getDescriptorSet();
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS, pipeline->GetCreateInfoPtr()->pLayout, 3, 1, &lightDescriptorSet});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, pipelineInfo.pipeline});

        for (auto &&e : _gameObjects) drawGameObject(cmdBuffer, e.get(), pipeline->GetCreateInfoPtr()->pLayout);

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }

    void setupImGui() {}
};
EXAMPLE_MAIN2(Hello, Shit::SampleCountFlagBits::BIT_4)