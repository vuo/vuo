/**
 * @file
 * ModuleScope implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "ModuleScope.hh"

ModuleScope::KnownModulesDirectories *ModuleScope::knownModulesDirectories = nullptr;
ModuleScope::KnownModuleCaches *ModuleScope::knownModuleCaches = nullptr;

ModuleScope::ModuleScope(Scope scope, string compositionDirName) :
	scope(scope),
	compositionDirName(compositionDirName)
{}

string ModuleScope::getCompositionDirName()
{
	return compositionDirName;
}

shared_ptr<VuoModuleCache> ModuleScope::getModuleCache()
{
	KnownModuleCaches *k = getKnownModuleCaches();

	auto i = k->moduleCaches.find(scope);
	if (i != k->moduleCaches.end())
	{
		auto j = i->second.find(compositionDirName);
		if (j != i->second.end())
			return j->second;
		else if (compositionDirName.empty() && i->second.size() == 1)
			return i->second.begin()->second;
	}

	throw VuoException("Unknown module cache");
}

string ModuleScope::getInstalledModulesPath()
{
	KnownModulesDirectories *k = getKnownModulesDirectories();

	string modulesDirectory;

	auto i = k->modulesDirectories.find(scope);
	if (i != k->modulesDirectories.end())
	{
		auto j = i->second.find(compositionDirName);
		if (j != i->second.end())
			modulesDirectory = j->second;
		else if (compositionDirName.empty() && i->second.size() == 1)
			modulesDirectory = i->second.begin()->second;
	}

	if (! modulesDirectory.empty())
	{
		// Create the directory if we have permission, but don't fail immediately if not;
		// wait and see if the test actually needs to read/write files in the directory.
		try
		{
			VuoFileUtilities::makeDir(modulesDirectory);
		}
		catch (VuoException &e) {}

		return modulesDirectory;
	}

	throw VuoException("Unknown modules directory");
}

VuoModuleCacheManifest ModuleScope::getManifest()
{
	string manifestPath = getModuleCache()->getManifestPath();
	VuoModuleCacheManifest manifest;
	manifest.readFromFile(manifestPath);
	return manifest;
}

string ModuleScope::registerCompositionDirectory(string compositionDirName)
{
	string compositionDir = VuoFileUtilities::makeTmpDir(compositionDirName);
	string compositionPath = compositionDir + "/composition.vuo";

	KnownModuleCaches *kc = getKnownModuleCaches();
	kc->moduleCaches[CompositionFamily][compositionDirName] = VuoModuleCache::newCache(compositionDir);
	kc->moduleCaches[Composition][compositionDirName] = VuoModuleCache::newCache(compositionPath);

	KnownModulesDirectories *kd = getKnownModulesDirectories();
	kd->modulesDirectories[CompositionFamily][compositionDirName] = compositionDir + "/Modules";
	kd->modulesDirectories[Composition][compositionDirName] = compositionDir + "/Modules";

	return compositionPath;
}

void ModuleScope::deleteAllInstalledTestModules()
{
	KnownModulesDirectories *k = getKnownModulesDirectories();
	for (auto i : k->modulesDirectories)
	{
		if (i.first == System || i.first == User)
		{
			for (VuoFileUtilities::File *f : VuoFileUtilities::findAllFilesInDirectory(i.second.begin()->second))
			{
				if (VuoStringUtilities::beginsWith(f->getRelativePath(), "vuo.test"))
					VuoFileUtilities::deleteFile(f->path());

				delete f;
			}
		}
		else
		{
			for (auto j : i.second)
			{
				string tmpDir, file, ext;
				VuoFileUtilities::splitPath(j.second, tmpDir, file, ext);
				VuoFileUtilities::deleteDir(tmpDir);
			}
		}
	}
}

bool operator<(const ModuleScope &lhs, const ModuleScope &rhs)
{
	return lhs.scope < rhs.scope;  // arbitrary comparison, just something to enable using std::map keyed on this type
}

ModuleScope::KnownModuleCaches::KnownModuleCaches()
{
	moduleCaches[System][""] = VuoModuleCache::newSystemCache();
	moduleCaches[User][""] = VuoModuleCache::newUserCache();
}

ModuleScope::KnownModuleCaches * ModuleScope::getKnownModuleCaches()
{
	if (! knownModuleCaches)
		knownModuleCaches = new KnownModuleCaches();

	return knownModuleCaches;
}

ModuleScope::KnownModulesDirectories::KnownModulesDirectories()
{
	modulesDirectories[System][""] = VuoFileUtilities::getSystemModulesPath();
	modulesDirectories[User][""] = VuoFileUtilities::getUserModulesPath();
}

ModuleScope::KnownModulesDirectories * ModuleScope::getKnownModulesDirectories()
{
	if (! knownModulesDirectories)
		knownModulesDirectories = new KnownModulesDirectories();

	return knownModulesDirectories;
}
