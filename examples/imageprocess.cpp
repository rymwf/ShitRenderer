#include "common/appbase.h"
#include "common/entry.h"
auto vertShaderPath = "imageprocess/display.vert";
auto fragShaderPath = "imageprocess/display.frag";
auto compShaderPath = "imageprocess/process.comp";

auto testImagePath = SHIT_SOURCE_DIR "/assets/images/grace-new2.hdr";
struct PipelineInfo {
    std::vector<Shader *> shaders;
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
struct ProcessPara {
    int mode;
    int kernelRadius;
    float *kernel;
};

struct DisplayPara {
    int channel;
    int mode;
    float saturationFactor;
    float exposure{1.};
    float factor0;
    float factor1;
    int intfactor0;
    int acesFilm;
    int quantityK{1};
};

class Hello : public AppBase {
    Image *testImage;
    Texture *inTexture;
    Texture *outTexture;

    DisplayPara displayPara{};
    Shit::DescriptorSet *descriptorSetInTex;
    Shit::DescriptorSet *descriptorSetOutTex;
    PipelineInfo displayPipelineInfo;
    Shit::DescriptorSet *currentDescriptorSet;

    PipelineInfo compPipelineInfo;
    Shit::DescriptorSet *compDescriptorSet;
    Shit::Buffer *compSSBO;
    ProcessPara compProcessPara{};
    FilterKernel compKernel;

public:
    void loadAssets() {
        testImage = static_cast<Image *>(_imageManager->create(testImagePath));
        inTexture = _textureManager->createOrRetrieveTexture(
            testImage, Shit::ImageUsageFlagBits::SAMPLED_BIT | Shit::ImageUsageFlagBits::TRANSFER_DST_BIT,
            Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
        inTexture->prepare();

        outTexture = _textureManager->createTexture(
            Shit::ImageCreateInfo{{},
                                  Shit::ImageType::TYPE_2D,
                                  // Shit::Format::R8G8B8A8_UNORM,
                                  Shit::Format::R32G32B32A32_SFLOAT,
                                  inTexture->getImage()->GetCreateInfoPtr()->extent,
                                  0,
                                  1,
                                  Shit::SampleCountFlagBits::BIT_1,
                                  Shit::ImageTiling::OPTIMAL,
                                  Shit::ImageUsageFlagBits::TRANSFER_DST_BIT | Shit::ImageUsageFlagBits::SAMPLED_BIT |
                                      Shit::ImageUsageFlagBits::STORAGE_BIT,
                                  Shit::MemoryPropertyFlagBits::DEVICE_LOCAL_BIT,
                                  Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            Shit::ImageViewType::TYPE_2D, 0);
        outTexture->prepare();
    }
    void prepareDisplayPipeline() {
        auto sampler = getSampler("linear");
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, &sampler}};
        auto descriptorSetLayout = _device->Create(Shit::DescriptorSetLayoutCreateInfo{1, &bindings[0]});

        // pipelina layout
        std::vector<Shit::DescriptorSetLayout *> descriptorSetLayouts{
            descriptorSetLayout,
        };
        Shit::PushConstantRange ranges[]{{Shit::ShaderStageFlagBits::FRAGMENT_BIT, 1, 0, sizeof(DisplayPara)}};
        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{(uint32_t)descriptorSetLayouts.size(),
                                                                             descriptorSetLayouts.data(), 1, ranges});

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
            },  // PipelineMultisampleStateCreateInfo`
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
    void prepareDisplayDescriptorSet() {
        auto setLayout = dynamic_cast<Shit::GraphicsPipeline *>(displayPipelineInfo.pipeline)
                             ->GetCreateInfoPtr()
                             ->pLayout->GetCreateInfoPtr()
                             ->pSetLayouts[0];
        _descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{1, &setLayout}, &descriptorSetInTex);
        _descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{1, &setLayout}, &descriptorSetOutTex);
        // write descriptor set
        Shit::DescriptorImageInfo imagesInfo[]{
            {0, inTexture->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {0, outTexture->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
        };
        Shit::WriteDescriptorSet writes[]{
            {descriptorSetInTex, 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
            {descriptorSetOutTex, 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[1]},
        };
        _device->UpdateDescriptorSets(writes);
        currentDescriptorSet = descriptorSetOutTex;
    }
    void prepareComputePipeline() {
        auto sampler = getSampler("linear");
        Shit::DescriptorSetLayoutBinding bindings[]{
            {0, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT, 1, &sampler},
            // {0, Shit::DescriptorType::STORAGE_IMAGE, 1,
            // Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {1, Shit::DescriptorType::STORAGE_IMAGE, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
            {2, Shit::DescriptorType::STORAGE_BUFFER, 1, Shit::ShaderStageFlagBits::COMPUTE_BIT},
        };
        auto descriptorSetLayout =
            _device->Create(Shit::DescriptorSetLayoutCreateInfo{std::size(bindings), &bindings[0]});

        auto pipelineLayout = _device->Create(Shit::PipelineLayoutCreateInfo{1, &descriptorSetLayout});

        compPipelineInfo.shaders = {
            static_cast<Shader *>(_shaderManager->create(buildShaderPath(_device, compShaderPath, _rendererVersion),
                                                         ResourceManager::DEFAULT_LOADER_NAME, {{"stage", "COMP"}}))};

        auto shaderStageCIs = compPipelineInfo.getStageCIs();
        compPipelineInfo.pipeline = _device->Create(Shit::ComputePipelineCreateInfo{
            shaderStageCIs[0],
            pipelineLayout,
        });
    }
    void prepareComputeDescriptorSet() {
        auto setLayout = dynamic_cast<Shit::ComputePipeline *>(compPipelineInfo.pipeline)
                             ->GetCreateInfoPtr()
                             ->pLayout->GetCreateInfoPtr()
                             ->pSetLayouts[0];
        _descriptorPool->Allocate(Shit::DescriptorSetAllocateInfo{1, &setLayout}, &compDescriptorSet);

        // prepare ssbo
        auto w = MAX_KERNEL_RADIUS * 2 + 1;
        compSSBO = _device->Create(Shit::BufferCreateInfo{
            {},
            offsetof(ProcessPara, kernel) + sizeof(float) * w * w,
            Shit::BufferUsageFlagBits::TRANSFER_DST_BIT | Shit::BufferUsageFlagBits::STORAGE_BUFFER_BIT,
            Shit::MemoryPropertyFlagBits::HOST_COHERENT_BIT | Shit::MemoryPropertyFlagBits::HOST_VISIBLE_BIT,
        });
        updateSSBO();
        Shit::DescriptorBufferInfo bufferInfo{compSSBO, 0, compSSBO->GetCreateInfoPtr()->size};

        Shit::DescriptorImageInfo imagesInfo[]{
            {0, inTexture->getImageView(), Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL},
            {0, outTexture->getImageView(), Shit::ImageLayout::GENERAL},
        };
        Shit::WriteDescriptorSet writes[]{
            {compDescriptorSet, 0, 0, 1, Shit::DescriptorType::COMBINED_IMAGE_SAMPLER, &imagesInfo[0]},
            // {compDescriptorSet, 0, 0, 1, Shit::DescriptorType::STORAGE_IMAGE,
            // &imagesInfo[0]},
            {compDescriptorSet, 1, 0, 1, Shit::DescriptorType::STORAGE_IMAGE, &imagesInfo[1]},
            {compDescriptorSet, 2, 0, 1, Shit::DescriptorType::STORAGE_BUFFER, 0, &bufferInfo},
        };
        _device->UpdateDescriptorSets(writes);
    }
    void updateSSBO() {
        void *data;
        compSSBO->MapMemory(0, compSSBO->GetCreateInfoPtr()->size, &data);
        auto radius = compKernel.getRadius();
        memcpy(data, &compProcessPara, offsetof(ProcessPara, kernelRadius));
        char *p = (char *)data + offsetof(ProcessPara, kernelRadius);
        memcpy(p, &radius, sizeof(int));
        p += sizeof(int);
        memcpy(p, compKernel.getKernel().data(), compKernel.getKernel().size() * sizeof(float));
        compSSBO->UnMapMemory();
    }

    void processImage() {
        _device->ExecuteOneTimeCommand([&](Shit::CommandBuffer *pCommandBuffer) {
            Shit::ImageMemoryBarrier barrier{
                Shit::AccessFlagBits::MEMORY_READ_BIT,
                Shit::AccessFlagBits::MEMORY_WRITE_BIT,
                Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                Shit::ImageLayout::GENERAL,
                ST_QUEUE_FAMILY_IGNORED,
                ST_QUEUE_FAMILY_IGNORED,
                outTexture->getImage(),
                {Shit::ImageAspectFlagBits::COLOR_BIT, 0, outTexture->getImage()->GetCreateInfoPtr()->mipLevels, 0, 1}};

            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                                      {},
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      1,
                                                                      &barrier});

            pCommandBuffer->BindPipeline(
                Shit::BindPipelineInfo{Shit::PipelineBindPoint::COMPUTE, compPipelineInfo.pipeline});
            pCommandBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
                Shit::PipelineBindPoint::COMPUTE,
                dynamic_cast<Shit::ComputePipeline *>(compPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout, 0, 1,
                &compDescriptorSet});
            // pCommandBuffer->PushConstants(Shit::PushConstantInfo{
            //	dynamic_cast<Shit::ComputePipeline
            //*>(compPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout,
            //	Shit::ShaderStageFlagBits::COMPUTE_BIT,
            //	0,
            //	sizeof(ProcessPara),
            //	&processPara});
            auto &&extent = inTexture->getImage()->GetCreateInfoPtr()->extent;
            auto a = extent.width * extent.height;
            pCommandBuffer->Dispatch({a / 1024 + uint32_t((a % 1024) > 0), 1, 1});

            barrier.srcAccessMask = Shit::AccessFlagBits::MEMORY_WRITE_BIT;
            barrier.dstAccessMask = Shit::AccessFlagBits::MEMORY_READ_BIT;
            barrier.oldImageLayout = Shit::ImageLayout::GENERAL;
            barrier.newImageLayout = Shit::ImageLayout::SHADER_READ_ONLY_OPTIMAL;

            pCommandBuffer->PipelineBarrier(Shit::PipelineBarrierInfo{Shit::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                      Shit::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                                      {},
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      0,
                                                                      1,
                                                                      &barrier});
        });
        // outTexture->getImage()->GenerateMipmap(
        //	Shit::Filter::LINEAR,
        //	Shit::ImageLayout::GENERAL,
        //	Shit::ImageLayout::GENERAL);
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

        cmdBuffer->BindPipeline(
            Shit::BindPipelineInfo{Shit::PipelineBindPoint::GRAPHICS, displayPipelineInfo.pipeline});
        cmdBuffer->PushConstants(Shit::PushConstantInfo{
            dynamic_cast<Shit::GraphicsPipeline *>(displayPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout,
            Shit::ShaderStageFlagBits::FRAGMENT_BIT, 0, sizeof(DisplayPara), &displayPara});
        cmdBuffer->BindDescriptorSets(Shit::BindDescriptorSetsInfo{
            Shit::PipelineBindPoint::GRAPHICS,
            dynamic_cast<Shit::GraphicsPipeline *>(displayPipelineInfo.pipeline)->GetCreateInfoPtr()->pLayout, 0, 1,
            &currentDescriptorSet});
        cmdBuffer->Draw(Shit::DrawIndirectCommand{3, 1, 0, 0});

        cmdBuffer->EndRenderPass();
        cmdBuffer->End();
    }
    void prepare() override {
        loadAssets();
        prepareComputePipeline();
        prepareComputeDescriptorSet();
        processImage();

        prepareDisplayPipeline();
        prepareDisplayDescriptorSet();
    }
    void setupImGui() {
        ImGui::Begin("config");
        static bool displayFlag = false;

        displayFlag |= ImGui::RadioButton("all", &displayPara.channel, 0);
        ImGui::SameLine();
        displayFlag |= ImGui::RadioButton("R", &displayPara.channel, 1);
        ImGui::SameLine();
        displayFlag |= ImGui::RadioButton("G", &displayPara.channel, 2);
        ImGui::SameLine();
        displayFlag |= ImGui::RadioButton("B", &displayPara.channel, 3);
        ImGui::SameLine();
        displayFlag |= ImGui::RadioButton("A", &displayPara.channel, 4);

        displayFlag |= ImGui::InputFloat("saturation", &displayPara.saturationFactor, 0.1f);
        displayFlag |= ImGui::InputFloat("exposure", &displayPara.exposure, 0.1f);
        displayFlag |= ImGui::Checkbox("acesfilm", (bool *)&displayPara.acesFilm);

        displayFlag |= ImGui::RadioButton("None", &displayPara.mode, 0);
        displayFlag |= ImGui::RadioButton("CMY", &displayPara.mode, 1);
        ImGui::SameLine();
        displayFlag |= ImGui::RadioButton("HSI", &displayPara.mode, 2);
        displayFlag |= ImGui::RadioButton("log", &displayPara.mode, 3);
        displayFlag |= ImGui::RadioButton("power", &displayPara.mode, 4);
        displayFlag |= ImGui::RadioButton("constrast smoothstep", &displayPara.mode, 5);
        displayFlag |= ImGui::RadioButton("intensity slice", &displayPara.mode, 6);
        displayFlag |= ImGui::RadioButton("quantization", &displayPara.mode, 7);

        if (displayPara.mode == 4) {
            displayFlag |= ImGui::SliderFloat("power factor", &displayPara.factor0, 0.f, 5.f);
        }
        if (displayPara.mode == 6) {
            displayFlag |= ImGui::RadioButton("channelR", &displayPara.intfactor0, 0);
            ImGui::SameLine();
            displayFlag |= ImGui::RadioButton("channelG", &displayPara.intfactor0, 1);
            ImGui::SameLine();
            displayFlag |= ImGui::RadioButton("channelB", &displayPara.intfactor0, 2);
            displayFlag |= ImGui::SliderFloat("boundrayMin", &displayPara.factor0, 0.f, 5.f);
            displayFlag |= ImGui::SliderFloat("boundrayMax", &displayPara.factor1, 0.f, 5.f);
        }
        if (displayPara.mode == 7) {
            displayFlag |= ImGui::SliderInt("quantityK", &displayPara.quantityK, 1, 8);
        }

        if (displayFlag) {
            needUpdateDefaultCommandBuffer();
            displayFlag = false;
        }
        ImGui::Separator();
        static bool flag = false;
        flag |= ImGui::RadioButton("copy", &compProcessPara.mode, 0);

        flag |= ImGui::RadioButton("edge enhencement sobel", &compProcessPara.mode, 1);
        flag |= ImGui::RadioButton("filter", &compProcessPara.mode, 2);
        if (compProcessPara.mode == 2) {
            ImGui::Separator();
            static int filterType = 0;

            ImGui::Text("filter type");
            ImGui::RadioButton("secondorder_derivative", &filterType, 0);
            ImGui::RadioButton("unsharp masking", &filterType, 1);
            ImGui::RadioButton("lowerpass", &filterType, 2);
            ImGui::RadioButton("highpass", &filterType, 3);
            ImGui::RadioButton("bandreject", &filterType, 4);
            ImGui::RadioButton("bandpass", &filterType, 5);

            ImGui::Separator();
            static int radius[2]{1, 1}, pattern[2]{1, 1}, num = 1;
            static float k[2]{1, 1};
            static FilterPattern filterPattern[2];

            if (filterType >= 4)
                num = 2;
            else
                num = 1;
            if (filterType == 1) {
                ImGui::SliderFloat("unsharp masking factor", &k[0], 0.f, 10.f);
            }
            if (filterType > 0) {
                for (int i = 0; i < num; ++i) {
                    static std::string str;
                    ImGui::Text("filter %d pattern choose", i);
                    str = "box" + std::to_string(i);
                    if (ImGui::RadioButton(str.data(), &pattern[i], 1)) {
                        filterPattern[i] = FilterPattern::BOX;
                    }
                    ImGui::SameLine();
                    str = "gaussian" + std::to_string(i);
                    if (ImGui::RadioButton(str.data(), &pattern[i], 2)) {
                        filterPattern[i] = FilterPattern::GAUSSIAN;
                    }
                    str = "filter radius" + std::to_string(i);
                    ImGui::InputInt(str.data(), &radius[i], 1.f);
                    radius[i] = (std::min)(radius[i], (int)MAX_KERNEL_RADIUS);
                    ImGui::Separator();
                }
            }
            if (ImGui::Button("do filter")) {
                switch (filterType) {
                    case 0:
                        compKernel = FilterKernel(FilterPattern::SECONDORDER_DERIVATIVE, radius[0]);
                        break;
                    case 1:
                        compKernel = (1 + k[0]) * FilterKernel() - k[0] * FilterKernel(filterPattern[0], radius[0]);
                        break;
                    case 2:
                        compKernel = FilterKernel(filterPattern[0], radius[0]);
                        break;
                    case 3:
                        compKernel = FilterKernel() - FilterKernel(filterPattern[0], radius[0]);
                        break;
                    case 4:
                        compKernel = FilterKernel() + FilterKernel(filterPattern[0], radius[0]) -
                                     FilterKernel(filterPattern[1], radius[1]);
                        break;
                    case 5:
                        compKernel =
                            -FilterKernel(filterPattern[0], radius[0]) + FilterKernel(filterPattern[1], radius[1]);
                        break;
                }
                flag = true;
            }
        }
        if (flag) {
            updateSSBO();
            processImage();
            flag = false;
        }
        ImGui::End();
    }
};
EXAMPLE_MAIN2(Hello)