/**
 * @file
 * VuoModuleInfoIterator implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoModuleInfoIterator.hh"
#include "VuoFileUtilities.hh"

/**
 * Constructs an iterator that will visit every item in @a allModuleInfos.
 *
 * If @a allModuleInfos contains both a compiled module and its override, the override will be returned.
 *
 * @a allModuleInfos must not be modified between now and when the caller is finished iterating through the modules.
 */
VuoModuleInfoIterator::VuoModuleInfoIterator(map<string, map<string, VuoModuleInfo *> > *allModuleInfos,
											 const string &overriddenCompiledModuleCachePath)
{
	this->allModuleInfos = allModuleInfos;
	this->overriddenCompiledModuleCachePath = overriddenCompiledModuleCachePath;
	hasSearchModuleKeys = false;
	initialize();
}

/**
 * Constructs an iterator that will visit only the items in @a allModuleInfos that match one of @a searchModuleKeys.
 *
 * If @a allModuleInfos contains both a compiled module and its override, the override will be returned.
 *
 * @a allModuleInfos must not be modified between now and when the caller is finished iterating through the modules.
 */
VuoModuleInfoIterator::VuoModuleInfoIterator(map<string, map<string, VuoModuleInfo *> > *allModuleInfos,
											 const string &overriddenCompiledModuleCachePath,
											 const set<string> &searchModuleKeys)
{
	this->allModuleInfos = allModuleInfos;
	this->overriddenCompiledModuleCachePath = overriddenCompiledModuleCachePath;
	this->searchModuleKeys = searchModuleKeys;
	hasSearchModuleKeys = true;
	initialize();
}

/**
 * Helper for constructors.
 */
void VuoModuleInfoIterator::initialize(void)
{
	currSearchPath = allModuleInfos->begin();
	hasCurrModuleKey = false;

	if (! overriddenCompiledModuleCachePath.empty())
	{
		auto i = allModuleInfos->find(overriddenCompiledModuleCachePath);
		if (i != allModuleInfos->end())
		{
			std::transform(i->second.begin(), i->second.end(),
						   std::inserter(overriddenModuleKeys, overriddenModuleKeys.begin()),
						   [](const pair<string, VuoModuleInfo *> &p) { return p.first; });
		}
	}
}

/**
 * Returns the next VuoModuleInfo to visit, or null if there are no more to visit.
 */
VuoModuleInfo * VuoModuleInfoIterator::next(void)
{
	for ( ; currSearchPath != allModuleInfos->end(); ++currSearchPath)
	{
		if (! hasCurrModuleKey)
		{
			currModuleKey = currSearchPath->second.begin();
			hasCurrModuleKey = true;
		}

		bool isOverriddenCompiledModuleCachePath = (currSearchPath->first == overriddenCompiledModuleCachePath);

		for ( ; currModuleKey != currSearchPath->second.end(); ++currModuleKey)
		{
			if ((! hasSearchModuleKeys || searchModuleKeys.find(currModuleKey->first) != searchModuleKeys.end()) &&
					(isOverriddenCompiledModuleCachePath || overriddenModuleKeys.find(currModuleKey->first) == overriddenModuleKeys.end()))
			{
				VuoModuleInfo *moduleInfo = currModuleKey->second;
				++currModuleKey;
				return moduleInfo;
			}
		}

		hasCurrModuleKey = false;
	}

	return NULL;
}
