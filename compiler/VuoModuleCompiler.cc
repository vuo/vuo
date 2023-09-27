/**
 * @file
 * VuoModuleCompiler implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCompiler.hh"

#include "VuoCompilerIssue.hh"
#include "VuoFileUtilities.hh"
#include <dlfcn.h>

/**
 * This is a pointer so we can ensure it's initialized before we load the plugin dylibs and they register themselves.
 */
map<string, VuoModuleCompiler::Factory> *VuoModuleCompiler::factories;

/**
 * Loads the plugin dylibs.
 */
void VuoModuleCompiler::loadAllModuleCompilers(void)
{
	factories = new map<string, VuoModuleCompiler::Factory>;

	vector<string> compilerPaths;
	string vuoFrameworkPath = VuoFileUtilities::getVuoFrameworkPath();
	if (vuoFrameworkPath.empty())
		compilerPaths.push_back(VUO_BUILD_DIR "/lib/libVuoCModuleCompiler.dylib");
	else
	{
		set<VuoFileUtilities::File *> files = VuoFileUtilities::findFilesInDirectory(vuoFrameworkPath + "/Helpers/ModuleCompiler", set<string>{"dylib"});
		for (VuoFileUtilities::File *file : files)
		{
			compilerPaths.push_back(file->path());
			delete file;
		}
	}

	for (const string &compilerPath : compilerPaths)
	{
		void *handle = dlopen(compilerPath.c_str(), RTLD_NOW);
		if (! handle)
			VUserLog("Warning: The module compiler '%s' couldn't be loaded : %s", compilerPath.c_str(), dlerror());
	}
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
		VUserLog("Warning: A VuoModuleCompiler for type '%s' is already registered.  Ignoring the duplicate.", type.c_str());
		return;
	}

	(*factories)[type] = factory;
}

/**
 * If a VuoModuleCompiler subclass that is able to handle @a type and @a sourcePath is registered,
 * creates an instance of that subclass. Otherwise, returns null.
 */
VuoModuleCompiler * VuoModuleCompiler::newModuleCompiler(const string &type, const string &moduleKey, const string &sourcePath,
														 const VuoModuleCompilerSettings &settings,
														 std::function<VuoCompilerType *(const string &)> getVuoType)
{
	static once_flag once;
	std::call_once(once, []() {
		loadAllModuleCompilers();
	});

	auto factory = factories->find(type);
	if (factory != factories->end())
	{
		VuoModuleCompiler *moduleCompiler = factory->second(moduleKey, sourcePath, settings);
		if (moduleCompiler)
		{
			moduleCompiler->getVuoType = getVuoType;
			return moduleCompiler;
		}
	}

	return nullptr;
}

/**
 * Constructor to be called by derived classes.
 */
VuoModuleCompiler::VuoModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings) :
	moduleKey(moduleKey),
	sourcePath(sourcePath),
	settings(settings)
{
}

VuoModuleCompiler::~VuoModuleCompiler(void)
{
}

/**
 * Throws an exception. Derived classes can override this function to support generating header file contents for a module.
 */
string VuoModuleCompiler::generateHeader(VuoCompilerIssues *issues)
{
	VuoCompilerIssue issue(VuoCompilerIssue::Error, "generating header", sourcePath, "", "This source file type doesn't use headers.");
	issue.setModuleKey(moduleKey);
	issues->append(issue);

	return "";
}

/**
 * Adds the VuoCompilerType with the given name to `vuoTypes`.
 *
 * @threadNoQueue{llvmQueue}
 */
VuoCompilerType * VuoModuleCompiler::lookUpVuoType(const string &typeName)
{
	VuoCompilerType *type = getVuoType(typeName);

	if (type)
		vuoTypes[typeName] = type;

	return type;
}
