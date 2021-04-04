/**
 * @file ShitModule.hpp
 * @author yangzs
 * @brief 
 * @version 0.1
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#pragma once
#include "ShitRendererPrerequisites.hpp"
#include "ShitNonCopyable.hpp"

#include <unordered_map>

namespace Shit
{

	class Module : public NonCopyable
	{
	protected:
		virtual std::string BuildModuleFileName(const char *moduleName) = 0;

	public:
		virtual void *LoadProc(const char *procName) = 0;
		virtual ~Module(){};
	};

	class ModuleManager
	{
		std::unordered_map<const char *, std::unique_ptr<Module>> sModuleTable;

	public:
		static ModuleManager *Get()
		{
			static ModuleManager instance;
			return &instance;
		}

		Module *LoadModule(const char *moduleName);
		Module *GetModule(const char *moduleName);
		void UnLoadModule(const char *moduleName);
	};

}