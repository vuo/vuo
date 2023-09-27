/**
 * @file
 * VuoLinkerInputs implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLinkerInputs.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerEnvironment.hh"
#include "VuoCompilerModule.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoModuleCacheRevision.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates an empty set of linker inputs.
 */
VuoLinkerInputs::VuoLinkerInputs(void)
{
}

/**
 * Adds the compiled objects to be linked in so that the resulting binary provides the given dependencies.
 *
 * @param dependencies The keys/names of modules, libraries, and frameworks to be linked in.
 * @param moduleCacheRevisions Module caches available to provide dependencies to speed up linking.
 * @param compiler The compiler object to use for locating modules and libraries.
 *
 * @threadNoQueue{VuoCompiler::environmentQueue}
 */
void VuoLinkerInputs::addDependencies(const set<string> &dependencies, const vector<shared_ptr<VuoModuleCacheRevision>> &moduleCacheRevisions,
									  VuoCompiler *compiler)
{
	vector<string> librarySearchPaths;
	compiler->applyToInstalledEnvironments([&librarySearchPaths](VuoCompilerEnvironment *env)
	{
		vector<string> envLibrarySearchPaths = env->getLibrarySearchPaths();
		librarySearchPaths.insert(librarySearchPaths.end(), envLibrarySearchPaths.begin(), envLibrarySearchPaths.end());
	});

	for (const string &dependency : dependencies)
	{
		shared_ptr<VuoModuleCacheRevision> foundInRevision = nullptr;
		for (shared_ptr<VuoModuleCacheRevision> revision : moduleCacheRevisions)
			if (revision->contains(dependency))
				foundInRevision = revision;

		if (foundInRevision)
		{
			if (foundInRevision->isBuiltIn())
				builtInCachedDependencies[foundInRevision].insert(dependency);
			else
				nonBuiltInCachedDependencies[foundInRevision].insert(dependency);
		}
		else
		{
			VuoCompilerModule *module = nullptr;
			bool isGenerated = false;
			compiler->applyToAllEnvironments([&dependency, &module, &isGenerated](VuoCompilerEnvironment *env)
			{
				VuoCompilerModule *foundModule = env->findModule(dependency);
				if (foundModule)
				{
					module = foundModule;
					isGenerated = env->isGenerated();
				}
			});

			if (module)
			{
				VuoCompilerNodeClass *nodeClass = dynamic_cast<VuoCompilerNodeClass *>(module);
				if (! (nodeClass && VuoCompilerSpecializedNodeClass::hasGenericPortTypes(nodeClass)) )  // Skip not-fully-specialized generic modules
				{
					string modulePath = module->getModulePath();
					if (! modulePath.empty() && ! nodeClass && ! isGenerated)
					{
						// If the module is an installed type or library not inside of a node set, we can link in its bitcode file.

						if (module->isBuiltIn() && ! isGenerated)
							builtInLibraries.insert(modulePath);
						else
							nonBuiltInLibraries.insert(modulePath);
					}
					else
					{
						// Otherwise, we have to link in the in-memory LLVM module, which is slower.
						// https://b33p.net/kosada/vuo/vuo/-/issues/12927

						if (module->isBuiltIn())
							builtInModules.insert(module->getModule());
						else
							nonBuiltInModules.insert(module->getModule());
					}

					if (module->isBuiltIn())
						builtInNonCachedModuleKeys.insert(dependency);
					else
						nonBuiltInNonCachedModuleKeys.insert(dependency);
				}
			}
			else
			{
				if (VuoStringUtilities::endsWith(dependency, ".framework"))
					frameworks.insert(dependency);
				else
				{
					string dependencyPath = compiler->getLibraryPath(dependency, librarySearchPaths);
					if (! dependencyPath.empty())
						externalLibraries.insert(dependencyPath);

					// On macOS 11, libc.dylib and libobjc.dylib are not present,
					// but we can still link since Vuo.framework includes the TBDs.
					else if (dependency != "c"
						  && dependency != "objc")
						VUserLog("Warning: Could not locate dependency '%s'.", dependency.c_str());
				}
			}
		}
	}
}

/**
 * Adds the libraries to link in for the Vuo runtime.
 */
void VuoLinkerInputs::addVuoRuntime(VuoCompiler *compiler)
{
	addDependencies({compiler->getRuntimeDependency()}, {}, compiler);
}

/**
 * Adds the libraries to link in for the main function that can accompany the Vuo runtime.
 */
void VuoLinkerInputs::addVuoRuntimeMain(VuoCompiler *compiler)
{
	addDependencies({compiler->getRuntimeMainDependency()}, {}, compiler);
}

/**
 * Returns the in-memory LLVM modules to be linked in. This excludes any modules that are to be linked in
 * some other way (bitcode file or module cache).
 */
set<Module *> VuoLinkerInputs::getModules(void) const
{
	set<Module *> modules;
	modules.insert(builtInModules.begin(), builtInModules.end());
	modules.insert(nonBuiltInModules.begin(), nonBuiltInModules.end());
	return modules;
}

/**
 * Returns the subset of VuoLinkerInputs::getModules() that belong to the compiler's built-in environments.
 */
set<Module *> VuoLinkerInputs::getModulesInBuiltInEnvironments(void) const
{
	return builtInModules;
}

/**
 * Returns the subset of VuoLinkerInputs::getModules() that belong to the compiler's non-built-in environments.
 */
set<Module *> VuoLinkerInputs::getModulesInNonBuiltInEnvironments(void) const
{
	return nonBuiltInModules;
}

/**
 * Adds modules to be linked in that belong to the compiler's built-in environments.
 */
void VuoLinkerInputs::addModulesInBuiltInEnvironments(const set<Module *> &modules)
{
	builtInModules.insert(modules.begin(), modules.end());
}

/**
 * Adds modules to be linked in that belong to the compiler's non-built-in environments.
 */
void VuoLinkerInputs::addModulesInNonBuiltInEnvironments(const set<Module *> &modules)
{
	nonBuiltInModules.insert(modules.begin(), modules.end());
}

/**
 * Returns the bitcode files, static libraries, and dynamic libraries to be linked in.
 */
set<string> VuoLinkerInputs::getLibraries(void) const
{
	set<string> libraries;

	libraries.insert(builtInLibraries.begin(), builtInLibraries.end());
	libraries.insert(nonBuiltInLibraries.begin(), nonBuiltInLibraries.end());
	libraries.insert(externalLibraries.begin(), externalLibraries.end());

	for (auto i : builtInCachedDependencies)
		libraries.insert(i.first->getDylibPath());
	for (auto i : nonBuiltInCachedDependencies)
		libraries.insert(i.first->getDylibPath());

	return libraries;
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that belong to the compiler's built-in environments.
 */
set<string> VuoLinkerInputs::getLibrariesInBuiltInEnvironments(void) const
{
	set<string> libraries;

	libraries.insert(builtInLibraries.begin(), builtInLibraries.end());
	for (auto i : builtInCachedDependencies)
		libraries.insert(i.first->getDylibPath());

	return libraries;
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that belong to the compiler's non-built-in environments.
 */
set<string> VuoLinkerInputs::getLibrariesInNonBuiltInEnvironments(void) const
{
	set<string> libraries;

	libraries.insert(nonBuiltInLibraries.begin(), nonBuiltInLibraries.end());
	for (auto i : nonBuiltInCachedDependencies)
		libraries.insert(i.first->getDylibPath());

	return libraries;
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that do not belong to any of the compiler's environments.
 */
set<string> VuoLinkerInputs::getExternalLibraries(void) const
{
	return externalLibraries;
}

/**
 * Adds libraries to be linked in that belong to the compiler's built-in environments.
 */
void VuoLinkerInputs::addLibrariesInBuiltInEnvironments(const set<string> &libraries)
{
	builtInLibraries.insert(libraries.begin(), libraries.end());
}

/**
 * Adds libraries to be linked in that belong to the compiler's non-built-in environments.
 */
void VuoLinkerInputs::addLibrariesInNonBuiltInEnvironments(const set<string> &libraries)
{
	nonBuiltInLibraries.insert(libraries.begin(), libraries.end());
}

/**
 * Adds libraries to be linked in that do not belong to any of the compiler's environments.
 */
void VuoLinkerInputs::addExternalLibraries(const set<string> &libraries)
{
	externalLibraries.insert(libraries.begin(), libraries.end());
}

/**
 * Adds libraries to be linked in that do not belong to any of the compiler's environments.
 */
void VuoLinkerInputs::addExternalLibraries(const vector<string> &libraries)
{
	externalLibraries.insert(libraries.begin(), libraries.end());
}

/**
 * Adds a library to be linked in that does not belong to any of the compiler's environments.
 */
void VuoLinkerInputs::addExternalLibrary(const string &library)
{
	externalLibraries.insert(library);
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that are dynamic libraries and belong to the
 * compiler's built-in environments.
 */
set<string> VuoLinkerInputs::getDylibsInBuiltInEnvironments(void) const
{
	set<string> dylibs = filterToDynamicLibraries(builtInLibraries);

	for (auto i : builtInCachedDependencies)
		dylibs.insert(i.first->getDylibPath());

	return dylibs;
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that are dynamic libraries and belong to the
 * compiler's non-built-in environments.
 */
set<string> VuoLinkerInputs::getDylibsInNonBuiltInEnvironments(void) const
{
	set<string> dylibs = filterToDynamicLibraries(nonBuiltInLibraries);

	for (auto i : nonBuiltInCachedDependencies)
		dylibs.insert(i.first->getDylibPath());

	return dylibs;
}

/**
 * Returns the subset of VuoLinkerInputs::getLibraries() that are dynamic libraries and do not belong to
 * any of the compiler's environments.
 */
set<string> VuoLinkerInputs::getExternalDylibs(void) const
{
	return filterToDynamicLibraries(externalLibraries);
}

/**
 * Returns the frameworks to be linked in.
 */
set<string> VuoLinkerInputs::getFrameworks(void) const
{
	return frameworks;
}

/**
 * Adds frameworks to be linked in.
 */
void VuoLinkerInputs::addFrameworks(const set<string> &frameworks)
{
	this->frameworks.insert(frameworks.begin(), frameworks.end());
}

/**
 * Returns the module keys of the Vuo modules that are to be linked in as LLVM modules or bitcode files
 * and belong to the compiler's built-in environments.
 */
set<string> VuoLinkerInputs::getNonCachedModuleKeysInBuiltInEnvironments(void) const
{
	return builtInNonCachedModuleKeys;
}

/**
 * Returns the module keys of the Vuo modules that are to be linked in as LLVM modules or bitcode files
 * and belong to the compiler's non-built-in environments.
 */
set<string> VuoLinkerInputs::getNonCachedModuleKeysInNonBuiltInEnvironments(void) const
{
	return nonBuiltInNonCachedModuleKeys;
}

/**
 * Returns the keys/names of the Vuo modules and other dependencies that the constructor found in the
 * module caches belonging to the compiler's built-in environments.
 */
map<shared_ptr<VuoModuleCacheRevision>, set<string>> VuoLinkerInputs::getCachedDependenciesInBuiltInEnvironments(void) const
{
	return builtInCachedDependencies;
}

/**
 * Returns the keys/names of the Vuo modules and other dependencies that the constructor found in the
 * module caches belonging to the compiler's non-built-in environments.
 */
map<shared_ptr<VuoModuleCacheRevision>, set<string>> VuoLinkerInputs::getCachedDependenciesInNonBuiltInEnvironments(void) const
{
	return nonBuiltInCachedDependencies;
}

/**
 * Logs information about this object.
 */
void VuoLinkerInputs::print() const
{
	for (auto i : getLibraries())                                                          VUserLog("getLibraries: %s", i.c_str());
	for (auto i : getLibrariesInBuiltInEnvironments())                                     VUserLog("getLibrariesInBuiltInEnvironments: %s", i.c_str());
	for (auto i : getLibrariesInNonBuiltInEnvironments())                                  VUserLog("getLibrariesInNonBuiltInEnvironments: %s", i.c_str());
	for (auto i : getExternalLibraries())                                                  VUserLog("getExternalLibraries: %s", i.c_str());
	for (auto i : getDylibsInBuiltInEnvironments())                                        VUserLog("getDylibsInBuiltInEnvironments: %s", i.c_str());
	for (auto i : getDylibsInNonBuiltInEnvironments())                                     VUserLog("getDylibsInNonBuiltInEnvironments: %s", i.c_str());
	for (auto i : getExternalDylibs())                                                     VUserLog("getExternalDylibs: %s", i.c_str());
	for (auto i : getFrameworks())                                                         VUserLog("getFrameworks: %s", i.c_str());
	for (auto i : getNonCachedModuleKeysInBuiltInEnvironments())                           VUserLog("getNonCachedModuleKeysInBuiltInEnvironments: %s", i.c_str());
	for (auto i : getNonCachedModuleKeysInNonBuiltInEnvironments())                        VUserLog("getNonCachedModuleKeysInNonBuiltInEnvironments: %s", i.c_str());
	for (auto i : getCachedDependenciesInBuiltInEnvironments())    for (auto j : i.second) VUserLog("getCachedDependenciesInBuiltInEnvironments: %s -- %s", i.first->getDylibPath().c_str(), j.c_str());
	for (auto i : getCachedDependenciesInNonBuiltInEnvironments()) for (auto j : i.second) VUserLog("getCachedDependenciesInNonBuiltInEnvironments: %s -- %s", i.first->getDylibPath().c_str(), j.c_str());
}

/**
 * Returns the subset of @a libraries that have the `.dylib` file extension.
 */
set<string> VuoLinkerInputs::filterToDynamicLibraries(const set<string> &libraries)
{
	auto isDylib = [](const string &library) { return VuoStringUtilities::endsWith(library, ".dylib"); };

	set<string> dynamicLibraries;
	std::copy_if(libraries.begin(), libraries.end(),
				 std::inserter(dynamicLibraries, dynamicLibraries.begin()),
				 isDylib);

	return dynamicLibraries;
}
