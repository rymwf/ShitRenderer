/**
 * @file ShitModule.cpp
 * @author yangzs
 * @brief
 * @version 0.1
 *
 * @copyright Copyright (c) 2021
 *
 */
#include "ShitModule.hpp"

#ifndef UNICODE
typedef std::string String;
#else
typedef std::wstring String;
#endif

namespace Shit {
static std::unordered_map<const char*, std::unique_ptr<Module>> sModuleTable;

Module* ModuleManager::GetModule(const char* moduleName) {
  if (sModuleTable.find(moduleName) == sModuleTable.end()) {
    return LoadModule(moduleName);
  }
  return sModuleTable[moduleName].get();
}

void ModuleManager::UnLoadModule(const char* moduleName) {
  sModuleTable.erase(moduleName);
}

}  // namespace Shit