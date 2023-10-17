#pragma once
#include "camera.h"
#include "common.h"
#include "environment.h"
#include "gameobject.h"
#include "glfw/glfw3.h"
#include "image.h"
#include "input.hpp"
#include "light.h"
#include "model.h"
#include "prerequisites.h"
#include "shader.h"
#include "transform.h"

#define MAX_FRAMES_IN_FLIGHT 2

class AppBase;

extern AppBase *g_App;

class AppBase {
protected:
    std::wstring _windowTitle;
    Shit::SampleCountFlagBits _sampleCount{Shit::SampleCountFlagBits::BIT_1};

    Shit::Device *_device;
    GLFWwindow *_window;
    Shit::Surface *_surface;

    Shit::RenderSystem *_renderSystem;
    Shit::RendererVersion _rendererVersion;

    Shit::Queue *_graphicsQueue;
    Shit::Queue *_presentQueue;
    Shit::Queue *_transferQueue;
    Shit::Queue *_computeQueue;

    Shit::CommandPool *_shortLiveCommandPool;
    Shit::CommandPool *_longLiveCommandPool;

    //=========================

    uint32_t _currentFrame{};
    Shit::Semaphore *_imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Semaphore *_renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    Shit::Fence *_inFlightFences[MAX_FRAMES_IN_FLIGHT];

    Timer _timer;
    uint64_t _frameDeltaTimeMs;

    // bool framebufferResized{true};

    int _framebufferWidth;
    int _framebufferHeight;

    //=========================
    std::unique_ptr<ShaderManager> _shaderManager;
    std::unique_ptr<ImageManager> _imageManager;

    std::unique_ptr<TextureManager> _textureManager;
    std::unique_ptr<MaterialDataBlockManager> _materialDataBlockManager;

    std::unique_ptr<ModelManager> _modelManager;

    //=========================
    // swapchains
    Shit::Swapchain *_swapchain;

    std::vector<Shit::DescriptorSetLayout *> _commonDescriptorSetLayouts;
    Shit::PipelineLayout *_commonPipelineLayout;

    Shit::DescriptorPool *_descriptorPool;

    Shit::DescriptorSetLayout *_transformDescriptorSetLayout;
    Shit::DescriptorSetLayout *_cameraDescriptorSetLayout;
    Shit::DescriptorSetLayout *_materialDescriptorSetLayout;
    Shit::DescriptorSetLayout *_lightDescriptorSetLayout;
    Shit::DescriptorSetLayout *_envDescriptorSetLayout;

    // default pass
    struct RenderTarget {
        std::vector<Shit::Image *> images;
        std::vector<Shit::ImageView *> imageViews;

        // std::vector<Shit::ImageView *> colors;
        // std::vector<Shit::ImageView *> depths;
        // std::vector<Shit::ImageView *> resolves;

        Shit::RenderPass *renderPass;
        std::vector<Shit::Framebuffer *> frameBuffers;
        std::vector<Shit::ClearValue> clearValues;
    } _defaultRenderTarget;

    std::unordered_map<std::string, Shit::Sampler *> _samplers;

    std::unique_ptr<Environment> _environment;

    // signals
    Shit::Signal<void()> _swapchainRecreateSignal;

    //
    std::vector<Shit::CommandBuffer *> _imguiCommandBuffers;

    //====================
    Shit::CommandBuffer *_defaultCommandBuffers[MAX_FRAMES_IN_FLIGHT];

    std::vector<Shit::CommandBuffer *> _submitCommandBuffers;

    std::vector<std::unique_ptr<GameObject>> _gameObjects;

    std::vector<Behaviour *> _behaviours;

    Camera *_editorCamera;

    bool _takeScreenShot = false;
    bool _needUpdateDefaultCommandBuffer = false;
    bool _needRecreateSwapchain = false;

    // gui
    void initImGui();
    void renderImGui(uint32_t imageIndex);
    void ImguiRecreateSwapchain();

    void initDefaultCommandBuffer();

    void initRenderSystem();

    void initManagers();

    void createQueues();

    void createSwapchain();
    void createCommandPool();
    void createSyncObjects();
    void createDescriptorPool();

    void updateScene();
    virtual void updateFrame(uint32_t imageIndex) {}

    void recreateSwapchain();

    void createEditorCamera();

    void createSamplers();

    void createDescriptorSetLayouts();

public:
    AppBase();

    virtual ~AppBase();

    void init(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle, Shit::SampleCountFlagBits sampleCount);

    constexpr uint32_t getFramesInFlight() const { return MAX_FRAMES_IN_FLIGHT; }
    constexpr uint32_t getCurrentFrame() const { return _currentFrame; }
    constexpr Shit::DescriptorPool *getDescriptorPool() const { return _descriptorPool; }
    constexpr Shit::Device *getDevice() const { return _device; }
    constexpr Shit::RendererVersion getRendererVersion() const { return _rendererVersion; }
    constexpr Shit::Swapchain *getSwapchain() const { return _swapchain; }

    constexpr Shit::Queue *getGraphicsQueue() const { return _graphicsQueue; }
    constexpr Shit::Queue *getTransferQueue() const { return _transferQueue; }
    constexpr Shit::Queue *getComputeQueue() const { return _computeQueue; }
    constexpr Shit::Queue *getPresentQueue() const { return _presentQueue; }

    void resize();

    constexpr void needUpdateDefaultCommandBuffer() { _needUpdateDefaultCommandBuffer = true; }

    void mainLoop();

    void getFramebufferSize(int &width, int &height);

    void run(Shit::RendererVersion rendererVersion, const wchar_t *windowTitle,
             Shit::SampleCountFlagBits sampleCount = Shit::SampleCountFlagBits::BIT_1);

    constexpr void setSampleCount(Shit::SampleCountFlagBits sampleCount) {
        _sampleCount = sampleCount;
        _needRecreateSwapchain = true;
    }

    void addRecreateSwapchainListener(Shit::Slot<void()> slot) { _swapchainRecreateSignal.Connect(slot); }
    void removeRecreateSwapchainListener(Shit::Slot<void()> slot) { _swapchainRecreateSignal.Disconnect(slot); }
    ImageManager *getImageManager() const { return _imageManager.get(); }
    ModelManager *getModelManager() const { return _modelManager.get(); }
    ShaderManager *getShaderManager() const { return _shaderManager.get(); }
    TextureManager *getTextureManager() const { return _textureManager.get(); }
    MaterialDataBlockManager *getMaterialDataBlockManager() { return _materialDataBlockManager.get(); }

    constexpr Shit::DescriptorSetLayout *getTransformDescriptorSetLayout() const {
        return _transformDescriptorSetLayout;
    }
    constexpr Shit::DescriptorSetLayout *getCameraDescriptorSetLayout() const { return _cameraDescriptorSetLayout; }
    constexpr Shit::DescriptorSetLayout *getMaterialDescriptorSetLayout() const { return _materialDescriptorSetLayout; }
    constexpr Shit::DescriptorSetLayout *getLightDescriptorSetLayout() const { return _lightDescriptorSetLayout; }
    constexpr Shit::DescriptorSetLayout *getEnvDescriptorSetLayout() const { return _envDescriptorSetLayout; }

    virtual void prepare() {}

    virtual void cleanUp() {}

    virtual void recordDefaultCommandBuffer(uint32_t imageIndex);

    virtual void setupSubmitCommandBuffers(uint32_t frameIndex);

    virtual void createDefaultRenderPass();

    virtual void setDefaultRenderPassClearValue();

    virtual void createDefaultRenderTarget();

    virtual void setupImGui();

    Shit::Sampler *createSampler(std::string_view name, Shit::SamplerCreateInfo const &createInfo);

    Shit::Sampler *getSampler(std::string_view name) const { return _samplers.at(std::string(name)); }

    void onFramebufferResize(int width, int height);
    void onMouseButton(int button, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onScroll(double xoffset, double yoffset);
    void onKeyboard(int key, int scancode, int action, int mods);

    virtual void onFramebufferResizeImpl(int width, int height) {}
    virtual void onMouseButtonImpl(int button, int action, int mods) {}
    virtual void onMouseMoveImpl(double xpos, double ypos) {}
    virtual void onScrollImpl(double xoffset, double yoffset) {}
    virtual void onKeyboardImpl(int key, int scancode, int action, int mods) {}
};
