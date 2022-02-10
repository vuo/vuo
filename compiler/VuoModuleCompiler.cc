/**
 * @file
 * VuoModuleCompiler implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCompiler.hh"

#include <dlfcn.h>

/**
 * This is a pointer so we can ensure it's initialized before we load the plugin dylibs and they register themselves.
 */
map<string, VuoModuleCompiler::Factory> *VuoModuleCompiler::factories;

/**
 * Loads the plugin dylibs.
 */
void __attribute__((constructor)) VuoModuleCompiler::init()
{
	factories = new map<string, VuoModuleCompiler::Factory>;

	auto modules = VuoFileUtilities::findFilesInDirectory(VuoFileUtilities::getVuoFrameworkPath() + "/Helpers/ModuleCompiler", set<string>{"dylib"});
	for (auto module : modules)
		dlopen(module->path().c_str(), RTLD_NOW);
}

/**
 * Registers a method that creates an instance of a subclass of VuoModuleCompiler.
 *
 * To be called by plugin dylibs.
 */
void VuoModuleCompiler::registerModuleCompiler(string type, VuoModuleCompiler::Factory factory)
{
	auto existingFactory = factories->find(type);
	if (existingFactory != factories->end())
	{
		VUserLog("Error: A VuoModuleCompiler for type '%s' is already registered.  Ignoring the duplicate.", type.c_str());
		return;
	}

	(*factories)[type] = factory;
}

/**
 * Creates an instance of a subclass of VuoModuleCompiler.
 */
VuoModuleCompiler *VuoModuleCompiler::newModuleCompiler(string type, const string &moduleKey, VuoFileUtilities::File *sourceFile)
{
	auto factory = factories->find(type);
	if (factory == factories->end())
	{
		VUserLog("Error creating compiler for '%s': Couldn't find a VuoModuleCompiler for type '%s'.", moduleKey.c_str(), type.c_str());
		return NULL;
	}

	return factory->second(moduleKey, sourceFile);
}

VuoModuleCompiler::~VuoModuleCompiler(void)
{
}
