/**
 * @file
 * VuoLinkerInputs interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;
class VuoModuleCacheRevision;

/**
 * A set of compiled objects (LLVM modules, LLVM bitcode files, object files, libraries, frameworks)
 * to pass to an invocation of Clang's linker.
 *
 * This class distinguishes between modules/libraries built-in to Vuo vs. other modules/libraries
 * to support live editing in the face of a limitation of Objective-C. The process of live editing
 * involves loading and unloading dynamic libraries. However, a dynamic library containing Objective-C
 * can't be unloaded. Some built-in modules/libraries do contain Objective-C, but since they're built-in,
 * they never need to be unloaded during live editing. We don't handle the case where a non-built-in
 * module/library contains Objective-C and needs to be unloaded during live editing.
 * (https://stackoverflow.com/questions/8793099/unload-dynamic-library-needs-two-dlclose-calls)
 */
class VuoLinkerInputs
{
public:
	VuoLinkerInputs(void);

	void addDependencies(const set<string> &dependencies, const vector<shared_ptr<VuoModuleCacheRevision>> &moduleCacheRevisions, VuoCompiler *compiler);
	void addVuoRuntime(VuoCompiler *compiler);
	void addVuoRuntimeMain(VuoCompiler *compiler);

	set<Module *> getModules(void) const;
	set<Module *> getModulesInBuiltInEnvironments(void) const;
	set<Module *> getModulesInNonBuiltInEnvironments(void) const;

	void addModulesInBuiltInEnvironments(const set<Module *> &modules);
	void addModulesInNonBuiltInEnvironments(const set<Module *> &modules);

	set<string> getLibraries(void) const;
	set<string> getLibrariesInBuiltInEnvironments(void) const;
	set<string> getLibrariesInNonBuiltInEnvironments(void) const;
	set<string> getExternalLibraries(void) const;

	void addLibrariesInBuiltInEnvironments(const set<string> &libraries);
	void addLibrariesInNonBuiltInEnvironments(const set<string> &libraries);
	void addExternalLibraries(const set<string> &libraries);
	void addExternalLibraries(const vector<string> &libraries);
	void addExternalLibrary(const string &library);

	set<string> getDylibsInBuiltInEnvironments(void) const;
	set<string> getDylibsInNonBuiltInEnvironments(void) const;
	set<string> getExternalDylibs(void) const;

	set<string> getFrameworks(void) const;

	void addFrameworks(const set<string> &frameworks);

	set<string> getNonCachedModuleKeysInBuiltInEnvironments(void) const;
	set<string> getNonCachedModuleKeysInNonBuiltInEnvironments(void) const;
	map<shared_ptr<VuoModuleCacheRevision>, set<string>> getCachedDependenciesInBuiltInEnvironments(void) const;
	map<shared_ptr<VuoModuleCacheRevision>, set<string>> getCachedDependenciesInNonBuiltInEnvironments(void) const;

	void print() const;

private:
	static set<string> filterToDynamicLibraries(const set<string> &libraries);

	set<Module *> builtInModules;
	set<Module *> nonBuiltInModules;

	set<string> builtInLibraries;
	set<string> nonBuiltInLibraries;
	set<string> externalLibraries;

	set<string> frameworks;

	set<string> builtInNonCachedModuleKeys;
	set<string> nonBuiltInNonCachedModuleKeys;
	map<shared_ptr<VuoModuleCacheRevision>, set<string>> builtInCachedDependencies;
	map<shared_ptr<VuoModuleCacheRevision>, set<string>> nonBuiltInCachedDependencies;
};
