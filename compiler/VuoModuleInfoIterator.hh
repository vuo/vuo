/**
 * @file
 * VuoModuleInfoIterator interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoModuleInfo;

/**
 * Efficiently iterates through collections of VuoModuleInfo objects in VuoCompilerEnvironment, without making copies of the collections.
 */
class VuoModuleInfoIterator
{
public:
	VuoModuleInfoIterator(map<string, map<string, VuoModuleInfo *> > *allModuleInfos, const string &overriddenCompiledModuleCachePath);
	VuoModuleInfoIterator(map<string, map<string, VuoModuleInfo *> > *allModuleInfos, const string &overriddenCompiledModuleCachePath, const set<string> &searchModuleKeys);
	VuoModuleInfo * next(void);

private:
	void initialize(void);

	map<string, map<string, VuoModuleInfo *> > *allModuleInfos;
	set<string> searchModuleKeys;
	bool hasSearchModuleKeys;
	map<string, map<string, VuoModuleInfo *> >::iterator currSearchPath;
	map<string, VuoModuleInfo *>::iterator currModuleKey;
	bool hasCurrModuleKey;
	string overriddenCompiledModuleCachePath;
	set<string> overriddenModuleKeys;
};
