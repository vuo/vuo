/**
 * @file
 * VuoModuleCacheRevision interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoModuleCacheManifest.hh"

/**
 * Tracks a module cache dylib from the time that this process either creates it or first reads it
 * to the time that this process finishes using it. Each caller that uses the dylib increments a reference count.
 * If the reference count reaches 0, indicating that the dylib has been replaced with a newer version,
 * this class deletes the dylib file.
 */
class VuoModuleCacheRevision
{
public:
	static shared_ptr<VuoModuleCacheRevision> createAndUse(const string &dylibPath, const VuoModuleCacheManifest &manifest, bool builtIn, bool mayDelete);
	void use(void);
	void disuse(void);

	string getDylibPath() const;
	bool contains(const string &content) const;
	bool isBuiltIn(void) const;

private:
	VuoModuleCacheRevision(const string &dylibPath, const VuoModuleCacheManifest &manifest, bool builtIn, bool mayDelete);

	string dylibPath;  ///< The path of the dylib for this revision of the module cache.
	VuoModuleCacheManifest manifest;  ///< The contents of this revision of the module cache.
	bool builtIn;  ///< True if this is the cache of built-in modules.
	bool mayDelete;   ///< True if this class is allowed to delete the module cache dylib, false if it's read-only.

	static std::mutex useCountMutex;  ///< Synchronizes access to #useCount.
	static map<string, int> useCount;  ///< The reference count for each dylib path.
};
