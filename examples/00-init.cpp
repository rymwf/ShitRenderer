#include "common/entry.h"

uint32_t WIDTH = 800, HEIGHT = 600;

class Hello {
    Shit::RenderSystem *renderSystem;
    GLFWwindow *window;

public:
    static void key_callback(GLFWwindow *pWindow, int key, int scancode, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(pWindow, GLFW_TRUE);
    }
    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, nullptr, nullptr);

        glfwSetKeyCallback(window, &Hello::key_callback);
    }
    void initRenderSystem() {
        Shit::RenderSystemCreateInfo renderSystemCreateInfo{g_RendererVersion,
                                                            // Shit::RendererVersion::VULKAN,
                                                            Shit::RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT};

        renderSystem = LoadRenderSystem(renderSystemCreateInfo);
    }

    /**
     * @brief process window event, do not write render code here
     *
     * @param ev
     */
    void ProcessEvent(const Shit::Event &ev) {}
    void mainLoop() {
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }
    }
    void cleanUp() {}
    void run() {
        initWindow();
        initRenderSystem();
        mainLoop();
        cleanUp();
    }
};
EXAMPLE_MAIN(Hello)