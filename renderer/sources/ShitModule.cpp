/**
 * @file ShitModule.cpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "ShitModule.h"

namespace Shit
{
	static std::unordered_map<const char *, std::unique_ptr<Module>> sModuleTable;
#ifdef _WIN32
#include <windows.h>
	class Win32Module : public Module
	{

		HINSTANCE mHDLL = NULL;

	public:
		Win32Module(const char *moduleName)
		{
			std::string fileName = BuildModuleFileName(moduleName);
			mHDLL = LoadLibrary(fileName.data());
			if (mHDLL == NULL)
			{
				throw std::runtime_error("file " + fileName + " not exist");
			}
		}
		~Win32Module()
		{
			FreeLibrary(mHDLL);
		}
		std::string BuildModuleFileName(const char *moduleName) override
		{
			std::string str = "";
			str += SHIT_OUTPUT_DIR;
			str += "/";
			str += moduleName;
#ifndef NDEBUG
			str += 'd';
#endif
			str += ".dll";
			return str;
		}
		void *LoadProc(const char *procName) override
		{
			return reinterpret_cast<void *>(GetProcAddress(mHDLL, procName));
		}
	};

#endif

	Module *ModuleManager::LoadModule(const char *moduleName)
	{
#ifdef _WIN32
		sModuleTable[moduleName] = std::unique_ptr<Module>(new Win32Module(moduleName));
		return sModuleTable[moduleName].get();
#endif //
	}

	Module *ModuleManager::GetModule(const char *moduleName)
	{
		if (sModuleTable.find(moduleName) == sModuleTable.end())
		{
			return LoadModule(moduleName);
		}
		return sModuleTable[moduleName].get();
	}

	void ModuleManager::UnLoadModule(const char *moduleName)
	{
		sModuleTable.erase(moduleName);
	}

}