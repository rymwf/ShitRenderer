#pragma once
#include <bitset>
#include <cstring>
#include <execution>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numeric>
#include <thread>
#include <variant>

#include "config.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "imguiimpl/imgui-impl.hpp"
#include "prerequisites.h"
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)
#define __WFILE__ WIDEN(__FILE__)

#define DEFAULT_WINDOW_X 80
#define DEFAULT_WINDOW_Y 80
#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600

#define COSTANT_ID_COMPUTE_LOCAL_SIZE_X 0
#define COSTANT_ID_COMPUTE_LOCAL_SIZE_Y 1
#define COSTANT_ID_COMPUTE_LOCAL_SIZE_Z 2
#define COSTANT_ID_COMPUTE_OPERATOR_DIM 3

template <typename T>
concept Function = std::is_function_v<T>;

class AppBase;

extern Shit::RendererVersion g_RendererVersion;
// struct AppInfo
//{
//	Shit::RenderSystem *pRenderSystem;
//	Shit::Device *pDevice;
//	Shit::Swapchain *pSwapchain;
//	uint32_t imageCount;
//	uint32_t currentImageIndex;
//	Shit::Window *pWindow;
// };
// extern AppInfo g_AppInfo;

extern Shit::ShadingLanguage g_shaderSourceLanguage;

#define ARR_SIZE(x) sizeof(x) / sizeof(x[0])

template <typename T, typename U = float, std::enable_if_t<std::is_integral_v<T>, bool> = true>
U intToFloat(T value) {
    return (std::max)(static_cast<U>(value) / (std::numeric_limits<std::decay_t<T>>::max)(), static_cast<U>(-1.f));
}

void *loadImage(const char *imagePath, int &width, int &height, int &components, int request_components,
                int &componentSize);
void saveImage(const char *imagePath, int width, int height, int component, const void *data, bool hdr = false);
void freeImage(void *pData);

// image layout cannot be undefined or preinitialized
void takeScreenshot(Shit::Device *pDevice, Shit::Image *pImage, Shit::ImageLayout imageLayout);
void saveImage(const char *dstPath, Shit::Device *pDevice, Shit::Image *pImage, Shit::ImageLayout imageLayout);

std::string compileGlslToSpirv(std::string_view glslcode, Shit::ShaderStageFlagBits stage,
                               Shit::RendererVersion renderVersion);

std::string readFile(const char *filename);
// std::string buildShaderPathGlsl(Shit::Device *pDevice, const char
// *shaderName, Shit::RendererVersion renderVersion);
std::string buildShaderPath(Shit::Device *pDevice, const char *shaderName, Shit::RendererVersion renderVersion);

Shit::SurfacePixelFormat chooseSwapchainFormat(const std::vector<Shit::SurfacePixelFormat> &candidates,
                                               Shit::Surface *surface);
Shit::PresentMode choosePresentMode(const std::vector<Shit::PresentMode> &candidates, Shit::Surface *surface);
Shit::ShadingLanguage chooseShaderShourceLanguage(const std::vector<Shit::ShadingLanguage> &shadingLanguages,
                                                  Shit::Device *pDevice);

void generateIrradianceMap(Shit::Device *pDevice, Shit::Image *pSrcImageView2D, Shit::ImageLayout srcInitialLayout,
                           Shit::ImageLayout srcFinalLayout, Shit::Image *pDstImageViewCube,
                           Shit::ImageLayout dstInitialLayout, Shit::ImageLayout dstFinalLayout);

void generateIrradianceMapSH(Shit::Device *pDevice, Shit::Image *pSrcImage2D, Shit::ImageLayout srcInitialLayout,
                             Shit::ImageLayout srcFinalLayout, Shit::Image *pDstImageCube,
                             Shit::ImageLayout dstInitialLayout, Shit::ImageLayout dstFinalLayout);

/**
 * @brief
 *
 * @param pDevice
 * @param spirvShaderName must be spirv file
 * @param pDstImage2D//must be spirv file
 * @param width group x size
 * @param height group x num
 * @param srcLayout
 * @param dstLayout
 */
void generateImage2D(Shit::Device *pDevice, const char *spirvShaderName, Shit::Image *pDstImage2D,
                     Shit::ImageLayout srcLayout, Shit::ImageLayout dstLayout);

/**
 * @brief
 *
 * @param pDevice
 * @param atmosphereThickness [0,5] default 1
 * @param lightDirection
 * @param pDstImage2D
 * @param srcLayout
 * @param dstLayout
 */
void generateEquirectangularProceduralSkybox(Shit::Device *pDevice, float atmosphereThickness, float lightDirection[3],
                                             Shit::Image *pDstImage2D, Shit::ImageLayout srcLayout,
                                             Shit::ImageLayout dstLayout);
/**
 * @brief
 *
 * @param pDevice
 * @param H0 atmosphere thickness km, for molecule 8km , for aerosols 1.2km
 * @param pDstImage2D
 * @param srcLayout
 * @param dstLayout
 */
void generateOpticalDepthImage(Shit::Device *pDevice, float H0, Shit::Image *pDstImage2D, Shit::ImageLayout srcLayout,
                               Shit::ImageLayout dstLayout);

enum class FilterPattern {
    DELTA,
    BOX,
    GAUSSIAN,
    SECONDORDER_DERIVATIVE,
};

//
class FilterKernel {
    std::vector<float> _kernel2D;

    static float gaussian(float r, float sigma) { return std::exp(-r * r / (2 * sigma * sigma)); }
    void delta();
    void gaussianKernel(uint32_t radius);
    void boxKernel(uint32_t radius);
    void secondorder_derivative();

public:
    FilterKernel() { delta(); }
    FilterKernel(FilterPattern filterType, uint32_t radius);

    constexpr std::span<float const> getKernel() const { return _kernel2D; }
    constexpr std::span<float> getKernel() { return _kernel2D; }
    uint32_t getRadius() const { return getWidth() / 2; }
    uint32_t getWidth() const { return std::sqrt(_kernel2D.size()); }

    FilterKernel operator+(FilterKernel const &other) const;
    FilterKernel operator-(FilterKernel const &other) const { return *this + (-other); }
    FilterKernel operator-() const {
        FilterKernel ret = *this;
        for (auto &e : ret._kernel2D) e *= -1;
        return ret;
    }
};
template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
FilterKernel operator*(T const &lhs, FilterKernel const &kernel) {
    FilterKernel ret = kernel;
    for (auto &e : ret.getKernel()) {
        e *= lhs;
    }
    return ret;
}
template <typename T, typename std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
FilterKernel operator/(T const &lhs, FilterKernel const &kernel) {
    FilterKernel ret = kernel;
    for (auto &e : ret.getKernel()) {
        e = lhs / e;
    }
    return ret;
}