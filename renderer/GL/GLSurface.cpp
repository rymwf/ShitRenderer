#include "GLSurface.hpp"

#include "GLDevice.hpp"
#include "GLRenderSystem.hpp"
#include "GLSwapchain.hpp"

#ifdef _WIN32
#include "Platform/Windows/ShitPrerequisitesWin32.hpp"
#endif

namespace Shit {
static void DebugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar *message, ST_MAYBE_UNUSED const void *userParam) {
    /*
  id: 131204
  Source: API :0x33350
  Type: Other :0x33361
  Severity: low :0x37192
  Texture state usage warning: The texture object (N) bound to texture image unit
  M does not have a defined base level and cannot be used for texture mapping.

  id: 131076
  Source: API :0x8246
  Type: Other :0x8251
  Severity: low :0x9148
  Usage warning: Generic vertex attribute array N uses a pointer with a small
  value (0x0000000000000000).Is this intended to be used as an offset into a
  buffer object?

  id: 131185
  Source: API :0x8246
  Type: Other :0x8251
  Severity: notification :0x826b
  Buffer detailed info: Buffer object N (bound to GL_UNIFORM_BUFFER (1), and
  GL_UNIFORM_BUFFER_EXT, usage hint is GL_DYNAMIC_DRAW) has been mapped WRITE_ONLY
  in SYSTEM HEAP memory (fast).

  id:131169
  Source: API :0x33350
  Type: Other :0x33361
  Severity: low :0x37192
  Framebuffer detailed info: The driver allocated storage for renderbuffer N.

  Debug message id:131154
  Type: Performance :0x33360
  Severity: medium :0x37191
  Source: API :0x33350
  length:81
  message:Pixel-path performance warning: Pixel transfer is synchronized with 3D
  rendering.

  Debug message id:131218
  Type: Performance :0x33360
  Severity: medium :0x37191
  Source: API :0x33350
  length:109
  message:Program/shader state performance warning: Fragment shader in program 4
  is being recompiled based on GL state

  Debug message id:131140
  Type: Other :0x33361
  Severity: low :0x37192
  Source: API :0x33350
  length:97
  message:Rasterization usage warning: Dithering is enabled, but is not supported
  for integer framebuffers.
  */
    if (id == 131204 || id == 131076 || id == 131185 || id == 131169 || id == 131154 || id == 131218 || id == 131140)
        return;  // ignore these non-significant error codes

    std::string str;
    str.reserve(512);
    str += "\nDebug message id:";
    str += std::to_string(id);
    str += "\n";
    switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            str += "Type: Error :0x";
            str += std::to_string(GL_DEBUG_TYPE_ERROR);
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            str += "Type: Deprecated Behaviour :0x";
            str += std::to_string(GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR);
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            str += "Type: Undefined Behaviour :0x";
            str += std::to_string(GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR);
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            str += "Type: Portability :0x";
            str += std::to_string(GL_DEBUG_TYPE_PORTABILITY);
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            str += "Type: Performance :0x";
            str += std::to_string(GL_DEBUG_TYPE_PERFORMANCE);
            break;
        case GL_DEBUG_TYPE_MARKER:
            str += "Type: Marker :0x";
            str += std::to_string(GL_DEBUG_TYPE_MARKER);
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            str += "Type: Push Group :0x";
            str += std::to_string(GL_DEBUG_TYPE_PUSH_GROUP);
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            str += "Type: Pop Group :0x";
            str += std::to_string(GL_DEBUG_TYPE_POP_GROUP);
            break;
        case GL_DEBUG_TYPE_OTHER:
            str += "Type: Other :0x";
            str += std::to_string(GL_DEBUG_TYPE_OTHER);
            break;
    }
    str += "\n";
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            str += "Severity: high :0x";
            str += std::to_string(GL_DEBUG_SEVERITY_HIGH);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            str += "Severity: medium :0x";
            str += std::to_string(GL_DEBUG_SEVERITY_MEDIUM);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            str += "Severity: low :0x";
            str += std::to_string(GL_DEBUG_SEVERITY_LOW);
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            str += "Severity: notification :0x";
            str += std::to_string(GL_DEBUG_SEVERITY_NOTIFICATION);
            break;
    }
    str += "\n";
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            str += "Source: API :0x";
            str += std::to_string(GL_DEBUG_SOURCE_API);
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            str += "Source: Window System :0x";
            str += std::to_string(GL_DEBUG_SOURCE_WINDOW_SYSTEM);
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            str += "Source: Shader Compiler :0x";
            str += std::to_string(GL_DEBUG_SOURCE_SHADER_COMPILER);
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            str += "Source: Third Party :0x";
            str += std::to_string(GL_DEBUG_SOURCE_THIRD_PARTY);
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            str += "Source: Application :0x";
            str += std::to_string(GL_DEBUG_SOURCE_APPLICATION);
            break;
        case GL_DEBUG_SOURCE_OTHER:
            str += "Source: Other :0x";
            str += std::to_string(GL_DEBUG_SOURCE_OTHER);
            break;
    }
    str += "\nlength:";
    str += std::to_string(length);
    str += "\nmessage:";
    str += message;
    ST_LOG(str);
}
void GLSurface::EnableDebugOutput(const void *userParam) {
    // enable OpenGL debug context if context allows for debug context
    GLint flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);  // makes sure errors are displayed
                                                // synchronously
        if (GLEW_VERSION_4_3) {
            glDebugMessageCallback(DebugOutputCallback, userParam);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
        } else {
#ifdef GL_ARB_debug_output
            glDebugMessageCallbackARB(DebugOutputCallback, userParam);
            glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#else
            ST_LOG("current opengl version do not support debug output");
#endif
        }
    } else {
        ST_LOG("current context do not support debug output");
    }
}
//==================================================================================
GLSurface::GLSurface(PhysicalDevice physicalDevice, GLSurface *shareSurface)
    : Surface(physicalDevice), mShareSurface(shareSurface) {
    mpStateManager = std::make_unique<GLStateManager>();
    mPresentQueueFamily = std::optional<QueueFamily>{
        {QueueFlagBits::COMPUTE_BIT | QueueFlagBits::GRAPHICS_BIT | QueueFlagBits::TRANSFER_BIT, 0, 1}};
}
void GLSurface::InitGL() {
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        ST_LOG("Error:", glewGetErrorString(err));
        ST_THROW("failed to init glew");
    }

    if (static_cast<bool>(g_RenderSystem->GetCreateInfo()->flags & RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT))
        EnableDebugOutput(this);
    //
    glGetIntegerv(GL_MAJOR_VERSION, &g_GLVersion.major);
    glGetIntegerv(GL_MINOR_VERSION, &g_GLVersion.minor);

    g_RenderSystem->SetGLVersion(
        Shit::RendererVersion(int(Shit::RendererVersion::GL) | (g_GLVersion.major << 8) | (g_GLVersion.minor << 4)));

    ST_LOG_VAR(g_GLVersion.major);
    ST_LOG_VAR(g_GLVersion.minor);

    SHIT_GL_110 = g_GLVersion.major * 10 + g_GLVersion.minor >= 11;
    SHIT_GL_120 = g_GLVersion.major * 10 + g_GLVersion.minor >= 12;
    SHIT_GL_121 = g_GLVersion.major * 10 + g_GLVersion.minor >= 13;
    SHIT_GL_130 = g_GLVersion.major * 10 + g_GLVersion.minor >= 13;
    SHIT_GL_140 = g_GLVersion.major * 10 + g_GLVersion.minor >= 14;
    SHIT_GL_150 = g_GLVersion.major * 10 + g_GLVersion.minor >= 15;
    SHIT_GL_200 = g_GLVersion.major * 10 + g_GLVersion.minor >= 20;
    SHIT_GL_210 = g_GLVersion.major * 10 + g_GLVersion.minor >= 21;
    SHIT_GL_300 = g_GLVersion.major * 10 + g_GLVersion.minor >= 30;
    SHIT_GL_310 = g_GLVersion.major * 10 + g_GLVersion.minor >= 31;
    SHIT_GL_320 = g_GLVersion.major * 10 + g_GLVersion.minor >= 32;
    SHIT_GL_330 = g_GLVersion.major * 10 + g_GLVersion.minor >= 33;
    SHIT_GL_400 = g_GLVersion.major * 10 + g_GLVersion.minor >= 40;
    SHIT_GL_410 = g_GLVersion.major * 10 + g_GLVersion.minor >= 41;
    SHIT_GL_420 = g_GLVersion.major * 10 + g_GLVersion.minor >= 42;
    SHIT_GL_430 = g_GLVersion.major * 10 + g_GLVersion.minor >= 43;
    SHIT_GL_440 = g_GLVersion.major * 10 + g_GLVersion.minor >= 44;
    SHIT_GL_450 = g_GLVersion.major * 10 + g_GLVersion.minor >= 45;
    SHIT_GL_460 = g_GLVersion.major * 10 + g_GLVersion.minor >= 46;
}
Swapchain *GLSurface::Create(const SwapchainCreateInfo &createInfo) {
    MakeCurrent();
    mSwapchain = std::make_unique<GLSwapchain>(this, createInfo);
    mShareSurface->MakeCurrent();
    return mSwapchain.get();
}
void GLSurface::GetPixelFormats(std::vector<SurfacePixelFormat> &formats) const {
    formats.clear();
#if WGL_ARB_framebuffer_sRGB | WGL_EXT_framebuffer_sRGB
    formats.emplace_back(SurfacePixelFormat{Format::R8G8B8A8_SRGB, ColorSpace::SRGB_NONLINEAR});
#endif
    formats.emplace_back(SurfacePixelFormat{Format::R8G8B8A8_UNORM, ColorSpace::SRGB_NONLINEAR});
}
void GLSurface::GetPresentModes(std::vector<PresentMode> &presentModes) const {
    presentModes.clear();
    presentModes.emplace_back(PresentMode::IMMEDIATE);
    presentModes.emplace_back(PresentMode::FIFO);
}

#ifdef _WIN32
#define LOADWGL                                                           \
    {                                                                     \
        GLenum err = wglewInit();                                         \
        if (GLEW_OK != err) {                                             \
            /* Problem: glewInit failed, something is seriously wrong. */ \
            ST_LOG("Error: ", glewGetErrorString(err));                   \
            ST_THROW("failed to init glew");                              \
        }                                                                 \
    }

// check extension supported
#define wglIsSupported(x) wglewIsSupported(x)

GLSurfaceWin32::GLSurfaceWin32(PhysicalDevice physicalDevice, GLSurface *shareSurface,
                               const SurfaceCreateInfoWin32 &createInfo)
    : GLSurface(physicalDevice, shareSurface) {
    if (createInfo.hwnd == 0)
        CreateBackgroundWindow();
    else
        mHwnd = (HWND)createInfo.hwnd;

    CreateContext();

    if (createInfo.hwnd == 0) {
        InitGL();
    }
}
GLSurfaceWin32::~GLSurfaceWin32() {
    // DestroyWindow();
    wglDeleteContext(mHRenderContext);
}
void GLSurfaceWin32::InitWglExtentions() {
    // create invisible window
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "aaa";

    if (!RegisterClass(&wc)) {
        ErrorExit((LPTSTR)TEXT("RegisterClass"));
        ST_THROW("Failed to register background OpenGL window.");
    }

    HWND dummy_window = CreateWindowEx(0, wc.lpszClassName, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                       CW_USEDEFAULT, 0, 0, wc.hInstance, 0);

    if (!dummy_window) {
        ErrorExit((LPTSTR)TEXT("CreateWindowEx"));
        ST_THROW("Failed to create dummy OpenGL window.");
    }

    HDC dummy_dc = GetDC(dummy_window);

    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),  // size of this pfd
        1,                              // version number
        PFD_DRAW_TO_WINDOW |            // support window
            PFD_SUPPORT_OPENGL |        // support OpenGL
            PFD_DOUBLEBUFFER,           // double buffered
        PFD_TYPE_RGBA,                  // RGBA type
        32,                             // 32-bit color depth framebuffer bits
        0,
        0,
        0,
        0,
        0,
        0,  // color bits ignored
        8,  // alpha bits
        0,  // shift bit ignored
        0,  // no accumulation buffer
        0,
        0,
        0,
        0,               // accum bits ignored
        24,              // 24-bit z-buffer
        8,               // 8-bit stencil buffer
        0,               // no auxiliary buffer
        PFD_MAIN_PLANE,  // main layer
        0,               // reserved
        0,
        0,
        0  // layer masks ignored
    };

    int pixel_format = ChoosePixelFormat(dummy_dc, &pfd);
    if (!pixel_format) {
        ST_THROW("Failed to find a suitable pixel format.");
    }
    if (!SetPixelFormat(dummy_dc, pixel_format, &pfd)) {
        ST_THROW("Failed to set the pixel format.");
    }

    HGLRC dummy_context = wglCreateContext(dummy_dc);
    if (!dummy_context) {
        ErrorExit((LPTSTR)TEXT("wglCreateContext"));
        ST_THROW("Failed to create a dummy OpenGL rendering context.");
    }

    if (!wglMakeCurrent(dummy_dc, dummy_context)) {
        ST_THROW("Failed to activate dummy OpenGL rendering context.");
    }

    LOADWGL

    wglMakeCurrent(dummy_dc, 0);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_dc);
    DestroyWindow(dummy_window);
}
void GLSurfaceWin32::CreateBackgroundWindow() {
    // main
    InitWglExtentions();

    // create invisible window
    WNDCLASS wc = {};
    wc.lpfnWndProc = DefWindowProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = "bbb";

    if (!RegisterClass(&wc)) {
        ErrorExit((LPTSTR)TEXT("RegisterClass"));
        ST_THROW("Failed to register background OpenGL window.");
    }

    mHwnd = CreateWindowEx(0, wc.lpszClassName, 0, 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                           wc.hInstance, 0);

    if (!mHwnd) {
        ErrorExit((LPTSTR)TEXT("CreateWindowEx"));
        ST_THROW("Failed to create dummy OpenGL window.");
    }
}
void GLSurfaceWin32::CreateContext() {
    // create render context
    mHdc = GetDC(mHwnd);
    //
    const int attribList[] = {
        WGL_DRAW_TO_WINDOW_ARB,
        GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,
        GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,
        GL_TRUE,
        WGL_ACCELERATION_ARB,
        WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,
        WGL_TYPE_RGBA_ARB,  // or WGL_TYPE_COLORINDEX_ARB
        WGL_COLOR_BITS_ARB,
        32,
        WGL_DEPTH_BITS_ARB,
        24,
        WGL_STENCIL_BITS_ARB,
        8,
        WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB,
        GL_TRUE,
        0,  // End
    };

    int pixelFormat{};
    UINT numFormats;
    if (!wglChoosePixelFormatARB(mHdc, attribList, NULL, 1, &pixelFormat, &numFormats)) {
        ST_THROW("wglChoosePixelFormatARB failed");
    }
    ST_LOG_VAR(pixelFormat);

    int majorversion = (std::max)((static_cast<int>(g_RenderSystem->GetCreateInfo()->version >> 8) & 0xf), 1);
    int minorversion = (static_cast<int>(g_RenderSystem->GetCreateInfo()->version) >> 4) & 0xf;

    int contexFlag{};
    // if (static_cast<bool>(g_RenderSystem->GetCreateInfo()->flags &
    // RenderSystemCreateFlagBits::CONTEXT_DEBUG_BIT))
    //     contexFlag |= WGL_CONTEXT_DEBUG_BIT_ARB;
    if (static_cast<bool>(g_RenderSystem->GetCreateInfo()->flags &
                          RenderSystemCreateFlagBits::GLCONTEXT_FORWARD_COMPATIBLE_BIT))
        contexFlag |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

    int profileMaskFlag{WGL_CONTEXT_CORE_PROFILE_BIT_ARB};
    if (static_cast<bool>(g_RenderSystem->GetCreateInfo()->flags &
                          RenderSystemCreateFlagBits::GLCONTEXT_COMPATIBILITY_PROFILE_BIT))
        profileMaskFlag |= WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;

    ST_LOG_VAR(majorversion);
    ST_LOG_VAR(minorversion);
    const int attribList2[] = {WGL_CONTEXT_MAJOR_VERSION_ARB,
                               majorversion,  // seems to be not support < 330
                               WGL_CONTEXT_MINOR_VERSION_ARB,
                               minorversion,
                               WGL_CONTEXT_FLAGS_ARB,
                               contexFlag,
                               WGL_CONTEXT_PROFILE_MASK_ARB,
                               profileMaskFlag,
                               0};

    PIXELFORMATDESCRIPTOR pfd;
    DescribePixelFormat(mHdc, pixelFormat, sizeof(pfd), &pfd);
    if (!SetPixelFormat(mHdc, pixelFormat, &pfd)) {
        ST_THROW("Failed to set the OpenGL 3.3 pixel format.");
    }
    HGLRC shareContext = 0;
    if (mShareSurface) shareContext = static_cast<GLSurfaceWin32 *>(mShareSurface)->GetRenderContext();

    mHRenderContext = wglCreateContextAttribsARB(mHdc, shareContext, attribList2);

    if (!mHRenderContext) {
        ErrorExit((LPTSTR)TEXT("wglCreateContextAttribsARB"));
        ST_THROW("failed to create render context");
    }
    ST_LOG("create render context succeed");

    MakeCurrent();

    SetSwapInterval(0);
}
void GLSurfaceWin32::MakeCurrent() const { wglMakeCurrent(mHdc, mHRenderContext); }
void GLSurfaceWin32::Swapbuffer() const { SwapBuffers(mHdc); }
void GLSurfaceWin32::SetSwapInterval(int interval) {
    if (wglIsSupported("WGL_EXT_swap_control")) {
        wglSwapIntervalEXT(interval);
    } else {
        ST_LOG("Warning, current wgl do not support WGL_EXT_swap_control");
    }
}
void GLSurfaceWin32::GetFramebufferSize(uint32_t &width, uint32_t &height) const {
    RECT rect;
    GetClientRect(mHwnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}
void GLSurfaceWin32::GetCapabilities(SurfaceCapabilities &caps) const {
    caps.minImageCount = 2;
    caps.maxImageCount = 8;
    GetFramebufferSize(caps.currentExtent.width, caps.currentExtent.height);
    caps.maxImageExtent = caps.minImageExtent = caps.currentExtent;
    caps.maxImageArrayLayers = 1;
}
#endif
}  // namespace Shit