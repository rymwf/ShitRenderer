#include "common/appbase.h"
#include "common/entry.h"
auto vertShaderPath = "bloom/display.vert";
auto fragShaderPath = "bloom/display.frag";
auto compShaderPath = "bloom/process.comp";

// auto testImagePath = SHIT_SOURCE_DIR "/assets/images/00000010.jpg";
auto testImagePath = SHIT_SOURCE_DIR "/assets/images/grace-new2.hdr";
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
#define MAX_KERNEL_RADIUS 20
#define LOD 4
struct DisplayPara {
    int mode;
    int acesFilm;
    int kernelRadius;
    float *kernel;
};

class Hello : public AppBase {
    Image *testImage;
    Texture *inTexture;

    DisplayPara displayPara{};
    PipelineInfo displayPipelineInfo;
    Shit::Buffer *displaySSBO;
    FilterKernel displayKernel;

    PipelineInfo compPipelineInfo;

public:
    void loadAssets() {
        testImage = static_cast<Image *>(_imageManager->create(testImagePath));
        inTexture = _textureManager->createOrRetrieveTexture(testImage,
                                                             Shit::ImageUsageFlagBits::SAMPLED_BIT |
                                                                 Shit::ImageUsageFlagBits::STORAGE_BIT |
                                                                 Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
                                                             Shit::ImageLayout::GENERAL);
        inTexture->prepare();
    }
    void prepareDisplayPipeline() {
        auto sampler = getSampler("linear");
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
            {1, Shit::DescriptorType::STORAGE_BUFFER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT},
        };
        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), bindings});

        displayPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, displayPipelineInfo.descriptorSets.data());

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            descriptorSetLayout,
        };
        displayPipelineInfo.pipelineLayout = _device->Create(
            Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(), descriptorSetLayouts.data()});

        //===============================
        displayPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, vertShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "VERT"}})),
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, fragShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "FRAG"}}))};

        auto shaderStageCIs = displayPipelineInfo.getStageCIs();

        Shit::PipelineColorBlendAttachmentState colorBlendAttachmentState{
            false,
        };
        colorBlendAttachmentState.colorWriteMask =
            Shit::ColorComponentFlagBits::R_BIT | Shit::ColorComponentFlagBits::G_BIT |
            Shit::ColorComponentFlagBits::B_BIT | Shit::ColorComponentFlagBits::A_BIT;

        Shit::DynamicState dynamicStates[]{Shit::DynamicState::VIEWPORT, Shit::DynamicState::SCISSOR};
        Shit::Viewport viewport{};
        Shit::Rect2D scissor{};

        displayPipelineInfo.pipeline = _device->Create(Shit::GraphicsPipelineCreateInfo{
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
            displayPipelineInfo.pipelineLayout,
            _defaultRenderTarget.renderPass,
            0});
    }
    void prepareDisplayDescriptorSet() {
        auto sampler = getSampler("linear");
        // write descriptor set
        Shit::DescriptorImageInfo imagesInfo[]{
            {sampler, inTexture->getImageView(), Shit::ImageLayout::GENERAL},
        };
        auto w = MAX_KERNEL_RADIUS * 2 + 1;
        displaySSBO = _device->Create(Shit::BufferCreateInfo{
            {},
            offsetof(DisplayPara, kernel) + sizeof(float) * w * w,
            Shit::BufferUsageFlagBits::TRANSFER_DST_BIT | Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT,
            Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
        });
        updateSSBO();
        Shit::DescriptorBufferInfo bufferInfo{displaySSBO, 0, displaySSBO->GetCreateInfoPtr()->size};

        Shit::WriteDescriptorSet writes[]{
            {displayPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER,
             &imagesInfo[0]},
            {displayPipelineInfo.descriptorSets[0], 1, 0, 1, Shit::DescriptorType::STORAGE_BUFFER, 0, &bufferInfo},
        };
        _device->UpdateDescriptorSets(writes);
    }
    void prepareComputePipeline() {
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };
        auto descriptorSetLayout =
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), &bindings[0]});

        compPipelineInfo.pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        compPipelineInfo.descriptorSets.resize(1);
        _descriptorPool->Allocate({1, &descriptorSetLayout}, compPipelineInfo.descriptorSets.data());

        compPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, compShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        auto shaderStageCIs = compPipelineInfo.getStageCIs();
        compPipelineInfo.pipeline = _device->Create(Shit::ComputePipelineCreateInfo{
            shaderStageCIs[0],
            compPipelineInfo.pipelineLayout,
        });
    }
    void prepareComputeDescriptorSet() {
        // prepare ssbo
        auto image = inTexture->getImage();
        auto imageView =
            _device->Create(Shit::ImageViewCreateInfo{image,
                                                      Shit::ImageViewType::TYPE_2D,
                                                      image->GetCreateInfoPtr()->format,
                                                      {},
                                                      {Shit::ImageAspectFlagBits::COLOR_BIT, LOD, 1, 0, 1}});

        Shit::DescriptorImageInfo imagesInfo[]{
            {0, imageView, Shit::ImageLayout::GENERAL},
        };
        Shit::WriteDescriptorSet writes[]{
            {compPipelineInfo.descriptorSets[0], 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[0]},
        };
        _device->UpdateDescriptorSets(writes);
    }
    void updateSSBO() {
        void *data;
        displaySSBO->MapMemory(0, displaySSBO->GetCreateInfoPtr()->size, &data);
        auto radius = displayKernel.getRadius();
        memcpy(data, &displayPara, offsetof(DisplayPara, kernelRadius));
        char *p = (char *)data + offsetof(DisplayPara, kernelRadius);
        memcpy(p, &radius, sizeof(int));
        p += sizeof(int);
        memcpy(p, displayKernel.getKernel().data(), displayKernel.getKernel().size() * sizeof(float));
        displaySSBO->UnMapMemory();
    }

    void recordDefaultCommandBuffer(uint32_t imageIndex) override {
        auto &&ext = _swapchain->GetCreateInfoPtr()->imageExtent;
        Shit::Viewport viewport{0, 0, ext.width, ext.height, 0, 1};
        Shit::Rect2D scissor{{}, ext};

        auto cmdBuffer = _defaultCommandBuffers[_currentFrame];
        cmdBuffer->Begin({Shit::CommandBufferUsageFlagBits::ONE_TIME_SUBMIT_BIT});

        cmdBuffer->GenerateMipmap(
            {inTexture->getImage(), Shit::Filter::LINEAR, Shit::ImageLayout::GENERAL, Shit::ImageLayout::GENERAL});

        cmdBuffer->BindPipeline(Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, compPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::COMPUTE,
            dynamic_cast<Shit::ComputePipeline *>(compPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout, 0, 1,
            &compPipelineInfo.descriptorSets[0]});

        auto &&extent = inTexture->getImage()->GetCreateInfoPtr()->extent;
        auto a = extent.width * extent.height / 64;
        cmdBuffer->Dispatch({a / 1024 + uint32_t((a % 1024) > 0), 1, 1});

        cmdBuffer->BeginRenderPass(Shit::BeginRenderPassInfo{_defaultRenderTarget.renderPass,
                                                             _defaultRenderTarget.frameBuffers[imageIndex],
                                                             {{}, _swapchain->GetCreateInfoPtr()->imageExtent},
                                                             (uint32_t)_defaultRenderTarget.clearValues.size(),
                                                             _defaultRenderTarget.clearValues.data(),
                                                             Shit::SubpassContents::INLINE});

        cmdBuffer->SetViewport(Shit::SetViewPortInfo{0, 1, &viewport});
        cmdBuffer->SetScissor(Shit::SetScissorInfo{0, 1, &scissor});

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, displayPipelineInfo.pipeline});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS,
            dynamic_cast<Shit::GraphicsPipeline *>(displayPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout, 0, 1,
            &displayPipelineInfo.descriptorSets[0]});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void prepare() override {
        loadAssets();
        prepareComputePipeline();
        prepareComputeDescriptorSet();

        prepareDisplayPipeline();
        prepareDisplayDescriptorSet();
    }
    void setupImGui() {
        ImGui::Begin("config");
        static bool flag = false;

        flag |= ImGui::Checkbox("aces", (bool *)&displayPara.acesFilm);

        flag |= ImGui::RadioButton("origin", &displayPara.mode, 0);
        flag |= ImGui::RadioButton("highlight", &displayPara.mode, 1);
        flag |= ImGui::RadioButton("blured highlight", &displayPara.mode, 2);
        flag |= ImGui::RadioButton("final", &displayPara.mode, 3);

        static int radius{1};
        flag |= ImGui::SliderInt("filter radius", &radius, 1, 5);
        radius = (std::min)(radius, (int)MAX_KERNEL_RADIUS);
        displayKernel = FilterKernel(FilterPattern::GAUSSIAN, radius);

        if (flag) {
            updateSSBO();
            flag = false;
        }
        ImGui::End();
    }
};
EXAMPLE_MAIN2(Hello)