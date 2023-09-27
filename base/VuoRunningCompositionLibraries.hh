/**
 * @file
 * VuoRunningCompositionLibraries interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * Keeps track of the dynamic libraries referenced by a composition during live coding.
 *
 * This class encapsulates the information passed between the compiler (when linking the composition)
 * and the runner, so that the caller can just pass the one object around.
 */
class VuoRunningCompositionLibraries
{
public:
	typedef std::function<void(void)> CallbackType;  ///< Type of callbacks to be called after loading/unloading events.

	VuoRunningCompositionLibraries(void);
	~VuoRunningCompositionLibraries(void);
	void enqueueResourceLibraryToLoad(const string &path, const set<string> &dependenciesInLibrary, bool isUnloadable);
	void enqueueResourceLibraryToUnload(const string &path);
	set<string> enqueueAllUnloadableResourceLibrariesToUnload(void);
	void enqueueCacheLibraryToLoad(const string &path, const set<string> &dependenciesInLibrary, bool isUnloadable, CallbackType doAfterLoadAttempted);
	set<string> enqueueCacheLibraryToUnload(const string &path);
	void enqueueLibraryContainingDependencyToUnload(const string &dependency);
	vector<string> dequeueLibrariesToLoad(vector<CallbackType> &doAfterLoadAttempted);
	vector<string> dequeueLibrariesToUnload(void);
	vector<string> getNonUnloadableLibrariesLoaded(void);
	vector<string> getUnloadableLibrariesLoaded(void);
	vector<string> getUnloadableResourceLibrariesLoaded(void);
	vector<string> getUnloadableCacheLibrariesLoaded(void);
	map<string, set<string>> getCacheLibrariesEnqueuedToUnload(void);
	set<string> getDependenciesLoaded(void);
	void addExternalLibraries(const set<string> &paths);
	void addExternalFrameworks(const set<string> &paths);
	set<string> getExternalLibraries(void);
	set<string> getExternalFrameworks(void);
	void setDeleteResourceLibraries(bool shouldDeleteResourceLibraries);

private:
	vector<string> resourcePathsToLoad;  ///< Resource dylibs created when linking and not yet loaded, in the order they were created (thus, sorted by dependencies).
	vector<string> cachePathsToLoad;  ///< Cache dylibs created by the compiler and not yet loaded, in the order they were created.
	vector<string> resourcePathsLoaded;  ///< Resource dylibs currently loaded, in the order they were created.
	vector<string> cachePathsLoaded;  ///< Cache dylibs currently loaded, in the order they were created.
	set<string> resourcePathsToUnload;  ///< Resource dylibs still loaded but earmarked to be unloaded.
	set<string> cachePathsToUnload;  ///< Cache dylibs still loaded but earmarked to be unloaded.
	map<string, bool> canUnloadPathToLoad;  ///< For each of the not-yet-loaded resource and cache dylibs, whether it can be unloaded.
	map<string, bool> canUnloadPathLoaded;  ///< For each of the currently-loaded resource and cache dylibs, whether it can be unloaded.
	map<string, set<string> > dependenciesToLoad;  ///< For each of the not-yet-loaded resource and cache dylibs, the node classes, etc. contained within it.
	map<string, set<string> > dependenciesLoaded;  ///< For each of the currently-loaded resource and cache dylibs, the node classes, etc. contained within it.
	set<string> externalLibraries;  ///< Dylibs not created by the Vuo compiler that are referenced by the resource and cache dylibs.
	set<string> externalFrameworks;  ///< Frameworks referenced by the resource and cache dylibs.
	bool shouldDeleteResourceLibraries;  ///< Whether resource dylib files should be deleted when the composition is finished using them.
	map<string, CallbackType> cachePathLoadedCallbacks;  ///< Callbacks to be called after the attempt has been made to load a module cache dylib or, if that never happens, when this VuoRunningCompositionLibraries is destroyed.
};
