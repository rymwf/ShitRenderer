#pragma once
#include "common/common.h"

#ifdef _WIN32
#include <combaseapi.h>
#endif

#ifdef _WIN32
inline void parseArgument(int ac, LPWSTR *av) {
    // for (int i = 0; i < ac; ++i)
    //{
    //	LOG(av[i]);
    // }
    if (ac > 1) {
        if (wcscmp(av[1], L"GL") == 0) {
            g_RendererVersion = Shit::RendererVersion::GL;
        } else  // if (wcscmp(av[1], L"VK") == 0)
        {
            g_RendererVersion = Shit::RendererVersion::VULKAN;
        }
    }
}

//#define CREATE_SCENE(A)                                                               \
//	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) \
//	{                                                                                 \
//		LPWSTR *argv;                                                                 \
//		int argc;                                                                     \
//		argv = CommandLineToArgvW(GetCommandLineW(), &argc);                          \
//		parseArgument(argc, argv);                                                    \
//		LocalFree(argv);                                                              \
//		try                                                                           \
//		{                                                                             \
//			AppBase app(1280, 720);                                                   \
//			app.run<A>();                                                             \
//		}                                                                             \
//		catch (const std::exception &e)                                               \
//		{                                                                             \
//			std::cout << e.what() << std::endl;                                       \
//		}                                                                             \
//	}

#define EXAMPLE_MAIN(A, ...)                                                            \
    int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) { \
        LPWSTR *argv;                                                                   \
        int argc;                                                                       \
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);                            \
        parseArgument(argc, argv);                                                      \
        LocalFree(argv);                                                                \
        try {                                                                           \
            A app;                                                                      \
            app.run(__VA_ARGS__);                                                       \
        } catch (const std::exception &e) {                                             \
            std::cout << e.what() << std::endl;                                         \
        }                                                                               \
    }

#define EXAMPLE_MAIN2(A, ...)                                                           \
    int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) { \
        LPWSTR *argv;                                                                   \
        int argc;                                                                       \
        argv = CommandLineToArgvW(GetCommandLineW(), &argc);                            \
        parseArgument(argc, argv);                                                      \
        LocalFree(argv);                                                                \
        try {                                                                           \
            A app;                                                                      \
            app.run(g_RendererVersion, __WFILE__, __VA_ARGS__);                         \
        } catch (const std::exception &e) {                                             \
            std::cout << e.what() << std::endl;                                         \
        }                                                                               \
    }

#else
inline void parseArgument(int ac, char **av) {
    for (int i = 0; i < ac; ++i) {
        std::cout << av[i] << std::endl;
    }
    if (ac > 1) {
        if (strcmp(av[1], "GL") == 0) {
            g_RendererVersion = Shit::RendererVersion::GL;
        } else  // if (strcmp(av[1], "VK") == 0)
        {
            g_RendererVersion = Shit::RendererVersion::VULKAN;
        }
    }
}
//#define CREATE_APP(A)                           \
//	int main(int argc, char **argv)             \
//	{                                           \
//		parseArgument(argc, argv);              \
//		try                                     \
//		{                                       \
//			AppBase app(1280, 720);             \
//			app.run<A>();                       \
//		}                                       \
//		catch (const std::exception &e)         \
//		{                                       \
//			std::cout << e.what() << std::endl; \
//		}                                       \
//	}

#define EXAMPLE_MAIN(A, ...)                      \
    int main(int ac, char **av) {                 \
        parseArgument(ac, av);                    \
        try {                                     \
            A app;                                \
            app.run(__VA_ARGS__);                 \
        } catch (const std::exception &err) {     \
            std::cout << err.what() << std::endl; \
        }

#define EXAMPLE_MAIN2(A, ...)                                   \
    int main(int ac, char **av) {                               \
        parseArgument(ac, av);                                  \
        try {                                                   \
            A app;                                              \
            app.run(s_rendererVersion, __WFILE__, __VA_ARGS__); \
        } catch (const std::exception &err) {                   \
            std::cout << err.what() << std::endl;               \
        }                                                       \
    }

#endif