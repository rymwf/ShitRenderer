#ifdef _WIN32
#include "Platform/Windows/ShitPrerequisitesWin32.hpp"
#include "ShitModule.hpp"

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

#include <strsafe.h>
void ErrorExit(LPTSTR lpszFunction) {
    // Retrieve the system error message for the last-error code
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(
        LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                    (LPTSTR)("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}

namespace Shit {
// Win32Module
class Win32Module : public Module {
    HINSTANCE mHDLL = NULL;

public:
    Win32Module(const char* moduleName) {
        std::string fileName = BuildModuleFileName(moduleName);
        mHDLL = LoadLibrary((LPCSTR)fileName.data());
        if (mHDLL == NULL) {
            auto str = "LoadLibrary " + fileName + " ";
            ErrorExit((LPTSTR)str.data());
            // ST_THROW("failed to load dll ", fileName.data(), "error code: ",
            // GetLastError());
        }
    }
    ~Win32Module() { FreeLibrary(mHDLL); }
    std::string BuildModuleFileName(const char* moduleName) override {
        std::string str = ExePath();
        str += "/";
        str += moduleName;
#ifndef NDEBUG
        str += 'd';
#endif
        str += ".dll";
        return str;
    }

    String ExePath() {
        TCHAR buffer[MAX_PATH] = {};
        GetModuleFileName(NULL, buffer, MAX_PATH);
        auto pos = String(buffer).find_last_of("\\/");
        return String(buffer).substr(0, pos);
    }

    void* LoadProc(const char* procName) override { return reinterpret_cast<void*>(GetProcAddress(mHDLL, procName)); }
};

Module* ModuleManager::LoadModule(const char* moduleName) {
    sModuleTable[moduleName] = std::unique_ptr<Module>(new Win32Module(moduleName));
    return sModuleTable[moduleName].get();
}

}  // namespace Shit
#endif
