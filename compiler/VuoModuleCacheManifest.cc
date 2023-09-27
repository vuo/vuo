/**
 * @file
 * VuoModuleCacheManifest implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleCacheManifest.hh"
#include "VuoFileUtilities.hh"
#include "VuoStringUtilities.hh"

/**
 * Constructs an empty manifest.
 */
VuoModuleCacheManifest::VuoModuleCacheManifest(void)
{
}

/**
 * Adds a module key to the manifest.
 */
void VuoModuleCacheManifest::addModule(const string &moduleKey, bool isOverridden)
{
	(isOverridden ? overriddenDependencies : dependencies).insert(moduleKey);
}

/**
 * Adds a dependency other than a module key to the manifest.
 */
void VuoModuleCacheManifest::addDependency(const string &dependency)
{
	dependencies.insert(dependency);
}

/**
 * Adds the contents of @a other to this manifest.
 */
void VuoModuleCacheManifest::addContentsOf(const VuoModuleCacheManifest &other)
{
	dependencies.insert(other.dependencies.begin(), other.dependencies.end());
	overriddenDependencies.insert(other.overriddenDependencies.begin(), other.overriddenDependencies.end());
}

/**
 * Returns true if this manifest contains exactly the same dependencies (with same overridden status) as @a other.
 */
bool VuoModuleCacheManifest::hasSameContentsAs(const VuoModuleCacheManifest &other) const
{
	return dependencies == other.dependencies && overriddenDependencies == other.overriddenDependencies;
}

/**
 * Adds the contents listed in @a manifestFilePath to this manifest.
 */
void VuoModuleCacheManifest::readFromFile(const string &manifestFilePath)
{
	string contents = VuoFileUtilities::readFileToString(manifestFilePath);
	vector<string> lines = VuoStringUtilities::split(contents, '\n');

	for (const string &line : lines)
	{
		vector<string> parts = VuoStringUtilities::split(line, ' ');
		if (parts.size() == 1)
			dependencies.insert(parts[0]);
		else if (parts.size() == 2 && parts[1] == "o")
			overriddenDependencies.insert(parts[0]);
	}
}

/**
 * Writes the list of contents in this manifest to @a manifestFilePath.
 */
void VuoModuleCacheManifest::writeToFile(const string &manifestFilePath) const
{
	vector<string> lines;

	for (const string &dep : overriddenDependencies)
		lines.push_back(dep + " o");

	lines.insert(lines.end(), dependencies.begin(), dependencies.end());

	string contents = VuoStringUtilities::join(lines, '\n');
	VuoFileUtilities::writeStringToFile(contents, manifestFilePath);
}

/**
 * Returns true if @a content is one of the module keys or other dependencies listed in this manifest.
 */
bool VuoModuleCacheManifest::contains(const string &content) const
{
	return dependencies.find(content) != dependencies.end() || overriddenDependencies.find(content) != overriddenDependencies.end();
}

/**
 * Returns a list of all module keys and dependencies names in this manifest.
 */
set<string> VuoModuleCacheManifest::getContents(void) const
{
	set<string> contents;

	contents.insert(dependencies.begin(), dependencies.end());
	contents.insert(overriddenDependencies.begin(), overriddenDependencies.end());

	return contents;
}
