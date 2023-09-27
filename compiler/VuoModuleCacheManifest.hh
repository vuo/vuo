/**
 * @file
 * VuoModuleCacheManifest interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoModuleInfoIterator.hh"

/**
 * A listing of modules and other dependencies contained in an actual or hypothetical VuoModuleCache.
 */
class VuoModuleCacheManifest
{
public:
	VuoModuleCacheManifest(void);

	void addModule(const string &moduleKey, bool isOverridden);
	void addDependency(const string &dependency);

	void addContentsOf(const VuoModuleCacheManifest &other);
	bool hasSameContentsAs(const VuoModuleCacheManifest &other) const;

	void readFromFile(const string &manifestFilePath);
	void writeToFile(const string &manifestFilePath) const;

	bool contains(const string &content) const;
	set<string> getContents(void) const;

private:
	set<string> dependencies;  ///< Module keys and dependency names contained in the manifest, excluding #overriddenDependencies.
	set<string> overriddenDependencies;  ///< Module keys contained in the manifest for modules whose source code has been overridden by VuoCompiler::overrideInstalledNodeClass.
};
