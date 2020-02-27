/**
 * @file
 * VuoRunningCompositionLibraries implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoRunningCompositionLibraries.hh"
#include "VuoFileUtilities.hh"

/**
 * Constructs an object that does not yet have any libraries.
 */
VuoRunningCompositionLibraries::VuoRunningCompositionLibraries(void)
{
	shouldDeleteResourceLibraries = false;
}

/**
 * Deletes the loaded resource library files.
 */
VuoRunningCompositionLibraries::~VuoRunningCompositionLibraries(void)
{
	if (shouldDeleteResourceLibraries)
		for (vector<string>::iterator i = resourcePathsLoaded.begin(); i != resourcePathsLoaded.end(); ++i)
			VuoFileUtilities::deleteFile(*i);
}

/**
 * Adds a resource library to the list of those pending to be loaded.
 *
 * If @a isUnloadable is true, the library can be unloaded during live coding. If false, it can't be unloaded,
 * e.g. because it contains Objective-C code.
 */
void VuoRunningCompositionLibraries::enqueueResourceLibraryToLoad(const string &path, const set<string> &dependenciesInLibrary,
																  bool isUnloadable)
{
	if (find(resourcePathsLoaded.begin(), resourcePathsLoaded.end(), path) == resourcePathsLoaded.end() ||
			find(resourcePathsToUnload.begin(), resourcePathsToUnload.end(), path) != resourcePathsToUnload.end())
	{
		resourcePathsToLoad.push_back(path);
		isPathUnloadable[path] = isUnloadable;
		dependenciesToLoad[path] = dependenciesInLibrary;
	}
}

/**
 * Adds a cache library to the list of those pending to be loaded.
 *
 * If @a isUnloadable is true, the library can be unloaded during live coding. If false, it can't be unloaded,
 * e.g. because it contains Objective-C code.
 */
void VuoRunningCompositionLibraries::enqueueCacheLibraryToLoad(const string &path, const set<string> &dependenciesInLibrary,
															   bool isUnloadable)
{
	if (find(cachePathsLoaded.begin(), cachePathsLoaded.end(), path) == cachePathsLoaded.end() ||
			find(cachePathsToUnload.begin(), cachePathsToUnload.end(), path) != cachePathsToUnload.end())
	{
		cachePathsToLoad.push_back(path);
		isPathUnloadable[path] = isUnloadable;
		dependenciesToLoad[path] = dependenciesInLibrary;
	}
}

/**
 * Adds the library containing @a dependency to the list of those pending to be unloaded.
 */
void VuoRunningCompositionLibraries::enqueueLibraryContainingDependencyToUnload(const string &dependency)
{
	for (map<string, set<string> >::iterator i = dependenciesLoaded.begin(); i != dependenciesLoaded.end(); ++i)
	{
		if (i->second.find(dependency) != i->second.end())
		{
			string libraryPath = i->first;

			if (! isPathUnloadable[libraryPath])
			{
				VUserLog("The library containing %s (%s) can't be unloaded.", dependency.c_str(), libraryPath.c_str());
				break;
			}

			if (find(resourcePathsLoaded.begin(), resourcePathsLoaded.end(), libraryPath) != resourcePathsLoaded.end())
				resourcePathsToUnload.insert(libraryPath);
			if (find(cachePathsLoaded.begin(), cachePathsLoaded.end(), libraryPath) != cachePathsLoaded.end())
				cachePathsToUnload.insert(libraryPath);

			break;
		}
	}
}

/**
 * Marks the libraries pending to be loaded as actually loaded.
 *
 * Returns those libraries, plus the already-loaded libraries that would have been unloaded by @ref dequeueLibrariesToUnload.
 * The returned libraries are ordered so that a library comes after the libraries it depends on.
 */
vector<string> VuoRunningCompositionLibraries::dequeueLibrariesToLoad(void)
{
	vector<string> libraryPathsToLoad;

	for (vector<string>::iterator i = cachePathsLoaded.begin(); i != cachePathsLoaded.end(); ++i)
		if (isPathUnloadable[*i])
			libraryPathsToLoad.push_back(*i);

	libraryPathsToLoad.insert(libraryPathsToLoad.end(), cachePathsToLoad.begin(), cachePathsToLoad.end());

	for (vector<string>::iterator i = resourcePathsLoaded.begin(); i != resourcePathsLoaded.end(); ++i)
		if (isPathUnloadable[*i])
			libraryPathsToLoad.push_back(*i);

	libraryPathsToLoad.insert(libraryPathsToLoad.end(), resourcePathsToLoad.begin(), resourcePathsToLoad.end());

	resourcePathsLoaded.insert(resourcePathsLoaded.end(), resourcePathsToLoad.begin(), resourcePathsToLoad.end());
	resourcePathsToLoad.clear();

	cachePathsLoaded.insert(cachePathsLoaded.end(), cachePathsToLoad.begin(), cachePathsToLoad.end());
	cachePathsToLoad.clear();

	dependenciesLoaded.insert(dependenciesToLoad.begin(), dependenciesToLoad.end());
	dependenciesToLoad.clear();

	return libraryPathsToLoad;
}

/**
 * Marks the libraries pending to be unloaded as actually unloaded.
 *
 * Returns those libraries, plus the still-loaded libraries that may depend on them and thus need to be unloaded
 * for these libraries to be unloaded successfully.
 * The returned libraries are ordered so that a library comes after the libraries that depend on it.
 */
vector<string> VuoRunningCompositionLibraries::dequeueLibrariesToUnload(void)
{
	for (set<string>::iterator i = resourcePathsToUnload.begin(); i != resourcePathsToUnload.end(); ++i)
	{
		dependenciesLoaded.erase(*i);
		isPathUnloadable.erase(*i);

		if (shouldDeleteResourceLibraries)
			VuoFileUtilities::deleteFile(*i);
	}

	for (int i = resourcePathsLoaded.size()-1; i >= 0; --i)
		if (find(resourcePathsToUnload.begin(), resourcePathsToUnload.end(), resourcePathsLoaded[i]) != resourcePathsToUnload.end())
			resourcePathsLoaded.erase(resourcePathsLoaded.begin() + i);

	for (int i = cachePathsLoaded.size()-1; i >= 0; --i)
		if (find(cachePathsToUnload.begin(), cachePathsToUnload.end(), cachePathsLoaded[i]) != cachePathsToUnload.end())
			cachePathsLoaded.erase(cachePathsLoaded.begin() + i);

	vector<string> libraryPathsToUnload;

	libraryPathsToUnload.insert(libraryPathsToUnload.end(), resourcePathsToUnload.rbegin(), resourcePathsToUnload.rend());

	for (vector<string>::reverse_iterator i = resourcePathsLoaded.rbegin(); i != resourcePathsLoaded.rend(); ++i)
		if (isPathUnloadable[*i])
			libraryPathsToUnload.push_back(*i);

	libraryPathsToUnload.insert(libraryPathsToUnload.end(), cachePathsToUnload.rbegin(), cachePathsToUnload.rend());

	for (vector<string>::reverse_iterator i = cachePathsLoaded.rbegin(); i != cachePathsLoaded.rend(); ++i)
		if (isPathUnloadable[*i])
			libraryPathsToUnload.push_back(*i);

	resourcePathsToUnload.clear();
	cachePathsToUnload.clear();

	return libraryPathsToUnload;
}

/**
 * Returns the currently-loaded libraries that can't be unloaded.
 */
vector<string> VuoRunningCompositionLibraries::getNonUnloadableLibrariesLoaded(void)
{
	vector<string> libraryPathsLoaded;

	for (vector<string>::iterator i = cachePathsLoaded.begin(); i != cachePathsLoaded.end(); ++i)
		if (! isPathUnloadable[*i])
			libraryPathsLoaded.push_back(*i);

	for (vector<string>::iterator i = resourcePathsLoaded.begin(); i != resourcePathsLoaded.end(); ++i)
		if (! isPathUnloadable[*i])
			libraryPathsLoaded.push_back(*i);

	return libraryPathsLoaded;
}

/**
 * Returns the currently-loaded libraries that can be unloaded and are not enqueued to be unloaded.
 */
vector<string> VuoRunningCompositionLibraries::getUnloadableLibrariesLoaded(void)
{
	vector<string> libraryPathsLoaded;

	for (vector<string>::iterator i = cachePathsLoaded.begin(); i != cachePathsLoaded.end(); ++i)
		if (isPathUnloadable[*i] && cachePathsToUnload.find(*i) == cachePathsToUnload.end())
			libraryPathsLoaded.push_back(*i);

	for (vector<string>::iterator i = resourcePathsLoaded.begin(); i != resourcePathsLoaded.end(); ++i)
		if (isPathUnloadable[*i] && resourcePathsToUnload.find(*i) == resourcePathsToUnload.end())
			libraryPathsLoaded.push_back(*i);

	return libraryPathsLoaded;
}

/**
 * Returns the node classes, etc. contained within all currently-loaded libraries that are not enqueued to be unloaded.
 */
set<string> VuoRunningCompositionLibraries::getDependenciesLoaded(void)
{
	set<string> dependenciesLoadedSet;

	set<string> libraryPathsLoaded;
	vector<string> nonUnloadable = getNonUnloadableLibrariesLoaded();
	vector<string> unloadable = getUnloadableLibrariesLoaded();
	libraryPathsLoaded.insert(nonUnloadable.begin(), nonUnloadable.end());
	libraryPathsLoaded.insert(unloadable.begin(), unloadable.end());

	for (set<string>::iterator i = libraryPathsLoaded.begin(); i != libraryPathsLoaded.end(); ++i)
	{
		map<string, set<string> >::iterator j = dependenciesLoaded.find(*i);
		if (j != dependenciesLoaded.end())
			dependenciesLoadedSet.insert(j->second.begin(), j->second.end());
	}

	return dependenciesLoadedSet;
}

/**
 * Stores libraries used by node classes, etc. so they don't have to be looked up again
 * each time the composition is re-linked.
 */
void VuoRunningCompositionLibraries::addExternalLibraries(const set<string> &paths)
{
	externalLibraries.insert(paths.begin(), paths.end());
}

/**
 * Stores frameworks used by node classes, etc. so they don't have to be looked up again
 * each time the composition is re-linked.
 */
void VuoRunningCompositionLibraries::addExternalFrameworks(const set<string> &paths)
{
	externalFrameworks.insert(paths.begin(), paths.end());
}

/**
 * Returns the libraries accumulated from @ref addExternalLibraries.
 */
set<string> VuoRunningCompositionLibraries::getExternalLibraries(void)
{
	return externalLibraries;
}

/**
 * Returns the frameworks accumulated from @ref addExternalFrameworks.
 */
set<string> VuoRunningCompositionLibraries::getExternalFrameworks(void)
{
	return externalFrameworks;
}

/**
 * Sets whether resource dylib files should be deleted when the composition is finished using them —
 * when they're marked as unloaded or when this object is destroyed.
 */
void VuoRunningCompositionLibraries::setDeleteResourceLibraries(bool shouldDeleteResourceLibraries)
{
	this->shouldDeleteResourceLibraries = shouldDeleteResourceLibraries;
}
