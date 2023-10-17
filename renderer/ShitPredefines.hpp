#pragma once

#ifdef SHIT_DLL
#define SHIT_API __declspec(dllexport)
#else
#define SHIT_API
#endif  // SHIT_DLL

#ifdef __GNUC__
#define LIBPREFIX "lib"
#else
#define LIBPREFIX ""
#endif

// standards
// c++98
#if __cplusplus >= 199711L
#define ST_HAS_CXX98
// c++11
#if __cplusplus >= 201103L
#define ST_HAS_CXX11
// C++14
#if __cplusplus >= 201402L
#define ST_HAS_CXX14
// C++17
#if __cplusplus >= 201703L
#define ST_HAS_CXX17
// C++20
#if __cplusplus >= 202002L
#define ST_HAS_CXX20
#endif
#endif
#endif
#endif
#endif

// compilers
#ifdef __MINGW32__
#define ST_MINGW
#define ST_MINGW32

#ifdef __MINGW64__
#define ST_MINGW64
#endif

#elif defined(_MSC_VER)
#define ST_MSVC
#elif defined(__clang__)
#define ST_CLANG
#elif defined(__GNUC__)
#define ST_GCC
#endif

// platform
#if defined(macintosh) || defined(Macintosh) || defined(__APPLE__)
#define ST_APPLE
#elif defined(__linux__)
#define ST_LINUX
#elif defined(__ANDROID__)
#define ST_ANDROID
#elif defined(_WIN32)
#define ST_WIN32
#endif

// architectures
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64) || defined(_M_AMD64)
#define ST_AMD64
#elif defined(__arm__) || defined(_M_ARM)
#define ST_ARM
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_X86_) || defined(_M_IX86)
#define ST_INTEL_X86
#endif

#ifdef __has_cpp_attribute

#if __has_cpp_attribute(carries_dependency)
#define ST_CARRIES_DEPENDENCY [[carries_dependency]]
#endif

#if __has_cpp_attribute(deprecated)
#define ST_DEPRECATED [[deprecated]]
#endif

#if __has_cpp_attribute(fallthrough)
#define ST_FALLTHROUGH [[fallthrough]]
#endif

#if __has_cpp_attribute(likely)
#define ST_LIKELY [[likely]]
#endif

#if __has_cpp_attribute(maybe_unused)
#define ST_MAYBE_UNUSED [[maybe_unused]]
#endif

#if __has_cpp_attribute(no_unique_address)
#define ST_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if __has_cpp_attribute(nodiscard)
#define ST_NODISCARD [[nodiscard]]
#endif

#if __has_cpp_attribute(noreturn)
#define ST_NORETURN [[noreturn]]
#endif

#if __has_cpp_attribute(unlikely)
#define ST_UNLIKELY [[unlikely]]
#endif

#else

#define ST_CARRIES_DEPENDENCY
#define ST_DEPRECATED
#define ST_FALLTHROUGH
#define ST_LIKELY
#define ST_MAYBE_UNUSED
#define ST_NO_UNIQUE_ADDRESS
#define ST_NODISCARD
#define ST_NORETURN
#define ST_UNLIKELY
#endif

#ifdef __cpp_constexpr
#define ST_CONSTEXPR constexpr
#else
#define ST_CONSTEXPR inline
#endif

#ifdef __cpp_consteval
#define ST_CONSTEVAL consteval
#else
#define ST_CONSTEVAL inline
#endif

#ifdef __cpp_constinit
#define ST_CONSTINIT constinit
#else
#define ST_CONSTINIT inline
#endif

//================================================================
#define SHIT_RENDERER_LOAD_FUNC "ShitLoadRenderSystem"
#define SHIT_RENDERER_DELETE_FUNC "ShitDeleteRenderSystem"

#define SHIT_RENDERER_GL_NAME "GLRenderer"
#define SHIT_RENDERER_D3D11_NAME "D3D11Renderer"
#define SHIT_RENDERER_D3D12_NAME "D3D12Renderer"
#define SHIT_RENDERER_METAL_NAME "MetalRenderer"
#define SHIT_RENDERER_VULKAN_NAME "VKRenderer"

#define ST_ATTACHMENT_UNUSED (~0U)
#define ST_QUEUE_FAMILY_IGNORED (~0U)
#define ST_SUBPASS_EXTERNAL (~0U)
#define ST_WHOLE_SIZE (~0ULL)

#ifdef NDEBUG
#define ST_LOG(...)
#define ST_LOG_VAR(str)
#else
#define ST_LOG(...)                                       \
    {                                                     \
        std::cout << __FILE__ << " " << __LINE__ << ": "; \
        Shit::myprint(std::cout, __VA_ARGS__);            \
        std::cout << std::endl;                           \
    }

#define ST_LOG_VAR(str) std::cout << __FILE__ << " " << __LINE__ << ":  " << #str << ": " << str << std::endl;
#endif

#define ST_THROW(...)                               \
    {                                               \
        std::stringstream ss;                       \
        Shit::myprint(ss, __FILE__, __LINE__, ":"); \
        Shit::myprint(ss, __VA_ARGS__);             \
        throw std::runtime_error(ss.str());         \
    }
