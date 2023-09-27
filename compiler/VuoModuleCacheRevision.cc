/**
 * @file
 * VuoModuleCacheRevision implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCacheRevision.hh"

#include "VuoException.hh"
#include "VuoFileUtilities.hh"

std::mutex VuoModuleCacheRevision::useCountMutex;
map<string, int> VuoModuleCacheRevision::useCount;

/**
 * Constructs a revision that is not yet reference-counted.
 */
VuoModuleCacheRevision::VuoModuleCacheRevision(const string &dylibPath, const VuoModuleCacheManifest &manifest, bool builtIn, bool mayDelete) :
	dylibPath(dylibPath),
	manifest(manifest),
	builtIn(builtIn),
	mayDelete(mayDelete)
{
}

/**
 * Constructs an object for tracking the module cache revision corresponding to @a dylibPath.
 *
 * Within a process, multiple VuoModuleCacheRevision instances may exist referring to the same dylib.
 * If this instance is the first, its reference count is 1 upon return from this function. Otherwise,
 * the reference count across all instances is incremented by 1.
 */
shared_ptr<VuoModuleCacheRevision> VuoModuleCacheRevision::createAndUse(const string &dylibPath, const VuoModuleCacheManifest &manifest,
																		bool builtIn, bool mayDelete)
{
	auto revision = shared_ptr<VuoModuleCacheRevision>(new VuoModuleCacheRevision(dylibPath, manifest, builtIn, mayDelete));

	std::lock_guard<std::mutex> lock(useCountMutex);

	if (useCount.find(dylibPath) == useCount.end())
		useCount[dylibPath] = 0;

	++useCount[dylibPath];

	return revision;
}

/**
 * Increments the reference count for this instance and any others referring to the same dylib.
 */
void VuoModuleCacheRevision::use(void)
{
	std::lock_guard<std::mutex> lock(useCountMutex);

	if (useCount[dylibPath] <= 0)
		throw VuoException("Error: Tried to use module cache dylib after it was deleted.");

	++useCount[dylibPath];
}

/**
 * Decrements the reference count for this instance and any others referring to the same dylib.
 *
 * If the reference count becomes 0, this function deletes the dylib file.
 */
void VuoModuleCacheRevision::disuse(void)
{
	std::lock_guard<std::mutex> lock(useCountMutex);

	if (useCount[dylibPath] <= 0)
		throw VuoException("Error: Tried to use module cache dylib after it was deleted.");

	--useCount[dylibPath];

	if (useCount[dylibPath] == 0)
	{
		if (mayDelete)
		{
			VDebugLog("Deleting no-longer-used module cache dylib: %s", dylibPath.c_str());
			VuoFileUtilities::deleteFile(dylibPath);
		}
		else
		{
			VUserLog("Warning: Tried to delete a read-only module cache dylib.");
		}
	}
}

/**
 * Returns the path of the dylib for this revision of the module cache.
 */
string VuoModuleCacheRevision::getDylibPath() const
{
	std::lock_guard<std::mutex> lock(useCountMutex);

	if (useCount[dylibPath] <= 0)
		throw VuoException("Error: Tried to use module cache dylib after it was deleted.");

	return dylibPath;
}

/**
 * Returns true if @a content is one of the module keys or other dependencies provided by this module cache revision.
 */
bool VuoModuleCacheRevision::contains(const string &content) const
{
	return manifest.contains(content);
}

/**
 * Returns true if this is the cache of built-in modules.
 */
bool VuoModuleCacheRevision::isBuiltIn(void) const
{
	return builtIn;
}
