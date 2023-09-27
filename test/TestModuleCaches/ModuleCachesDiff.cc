/**
 * @file
 * ModuleCachesDiff implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "ModuleCachesDiff.hh"

string ModuleCachesDiff::targetArch;

map<string, double> ModuleCachesDiff::listCompiledModulesInAllModuleCaches()
{
	map<string, double> compiledModules;

	for (VuoFileUtilities::File *moduleCacheDir : VuoFileUtilities::findAllFilesInDirectory(VuoFileUtilities::getCachePath()))
	{
		if (VuoFileUtilities::dirExists(moduleCacheDir->path()))
		{
			vector<string> scopeDirs;

			if (VuoStringUtilities::beginsWith(moduleCacheDir->getRelativePath(), "pid-"))
			{
				set<VuoFileUtilities::File *> scopeDirFiles = VuoFileUtilities::findAllFilesInDirectory(moduleCacheDir->path());
				for (VuoFileUtilities::File *f : scopeDirFiles)
				{
					scopeDirs.push_back(f->path());
					delete f;
				}
			}
			else
			{
				scopeDirs = { moduleCacheDir->path() };
			}

			for (string scopeDir : scopeDirs)
			{
				string installedDir = scopeDir + "/" + VuoModuleCache::getRelativePathOfModulesDirectory(false, targetArch);
				string generatedDir = scopeDir + "/" + VuoModuleCache::getRelativePathOfModulesDirectory(true, targetArch);

				set<VuoFileUtilities::File *> compiledModuleFiles;
				set<VuoFileUtilities::File *> installed = VuoFileUtilities::findAllFilesInDirectory(installedDir);
				set<VuoFileUtilities::File *> generated = VuoFileUtilities::findAllFilesInDirectory(generatedDir);
				compiledModuleFiles.insert(installed.begin(), installed.end());
				compiledModuleFiles.insert(generated.begin(), generated.end());

				for (VuoFileUtilities::File *f : compiledModuleFiles)
				{
					if (f->extension() != "d")
						compiledModules[f->path()] = VuoFileUtilities::getFileLastModifiedInSeconds(f->path());

					delete f;
				}
			}
		}

		delete moduleCacheDir;
	}

	return compiledModules;
}

map<string, double> ModuleCachesDiff::listDylibsInAllModuleCaches(vector<string> &moduleCachesWithMultipleDylibs)
{
	map<string, double> dylibs;

	for (VuoFileUtilities::File *moduleCacheDir : VuoFileUtilities::findAllFilesInDirectory(VuoFileUtilities::getCachePath()))
	{
		if (VuoFileUtilities::dirExists(moduleCacheDir->path()))
		{
			set<VuoFileUtilities::File *> dylibFiles = VuoFileUtilities::findFilesInDirectory(moduleCacheDir->path(), {"dylib"});

			if (dylibFiles.size() > 1)
				moduleCachesWithMultipleDylibs.push_back(moduleCacheDir->path());

			for (VuoFileUtilities::File *f : dylibFiles)
			{
				dylibs[f->path()] = VuoFileUtilities::getFileLastModifiedInSeconds(f->path());

				delete f;
			}
		}

		delete moduleCacheDir;
	}

	return dylibs;
}

void ModuleCachesDiff::recordBaseline()
{
	baselineCompiledModules = listCompiledModulesInAllModuleCaches();

	vector<string> moduleCachesWithMultipleDylibs;
	baselineDylibs = listDylibsInAllModuleCaches(moduleCachesWithMultipleDylibs);
	QVERIFY2(moduleCachesWithMultipleDylibs.empty(), ("module caches contain more than 1 .dylib: " + VuoStringUtilities::join(moduleCachesWithMultipleDylibs, " ")).c_str());
}

void ModuleCachesDiff::check()
{
	// Compiled modules

	QStringList actualCompiledModulesAdded;
	QStringList actualCompiledModulesModified;
	QStringList actualCompiledModulesRemoved;

	auto replaceKindDirectory = [](string compiledModulePath)
	{
		string replacedPath = compiledModulePath;
		VuoStringUtilities::replaceAll(replacedPath, "/Installed/", "/*/");
		VuoStringUtilities::replaceAll(replacedPath, "/Generated/", "/*/");
		return QString::fromStdString(replacedPath);
	};

	map<string, double> actualCompiledModules = listCompiledModulesInAllModuleCaches();

	for (auto i : actualCompiledModules)
	{
		auto baselineIter = baselineCompiledModules.find(i.first);

		if (baselineIter != baselineCompiledModules.end())
		{
			if (i.second > baselineIter->second)
				actualCompiledModulesModified.append(replaceKindDirectory(i.first));

			baselineCompiledModules.erase(baselineIter);
		}
		else
		{
			actualCompiledModulesAdded.append(replaceKindDirectory(i.first));
		}
	}

	for (auto i : baselineCompiledModules)
		actualCompiledModulesRemoved.append(replaceKindDirectory(i.first));

	QStringList actualOverriddenCompiledModulesAdded;
	QStringList actualOverriddenCompiledModulesModified;
	QStringList actualOverriddenCompiledModulesRemoved;

	auto replacePidDirectory = [](QString compiledModulePath)
	{
		return compiledModulePath.replace(QRegularExpression("\\/pid\\-[0-9]+\\/"), "/pid-*/");
	};

	auto separateOverriddenModules = [replacePidDirectory](QStringList &compiledModules, QStringList &overriddenCompiledModules)
	{
		for (int i = compiledModules.size() - 1; i >= 0; --i)
		{
			if (compiledModules.at(i).contains("/pid-"))
			{
				QString compiledModulePath = compiledModules.takeAt(i);
				overriddenCompiledModules.append(replacePidDirectory(compiledModulePath));
			}
		}
	};

	separateOverriddenModules(actualCompiledModulesAdded, actualOverriddenCompiledModulesAdded);
	separateOverriddenModules(actualCompiledModulesModified, actualOverriddenCompiledModulesModified);
	separateOverriddenModules(actualCompiledModulesRemoved, actualOverriddenCompiledModulesRemoved);

	QStringList expectedCompiledModulesAdded;
	QStringList expectedCompiledModulesModified;
	QStringList expectedCompiledModulesRemoved;

	for (ModuleLocation m : compiledModulesAdded)
		expectedCompiledModulesAdded.append(replaceKindDirectory(m.getCachedCompiledModulePath(false, targetArch)));

	for (ModuleLocation m : compiledModulesModified)
		expectedCompiledModulesModified.append(replaceKindDirectory(m.getCachedCompiledModulePath(false, targetArch)));

	for (ModuleLocation m : compiledModulesRemoved)
		expectedCompiledModulesRemoved.append(replaceKindDirectory(m.getCachedCompiledModulePath(false, targetArch)));

	QStringList expectedOverriddenCompiledModulesAdded;
	QStringList expectedOverriddenCompiledModulesModified;
	QStringList expectedOverriddenCompiledModulesRemoved;

	for (ModuleLocation m : overriddenCompiledModulesAdded)
		expectedOverriddenCompiledModulesAdded.append(replacePidDirectory(replaceKindDirectory(m.getCachedOverriddenCompiledModulePath(false, targetArch))));

	for (ModuleLocation m : overriddenCompiledModulesModified)
		expectedOverriddenCompiledModulesModified.append(replacePidDirectory(replaceKindDirectory(m.getCachedOverriddenCompiledModulePath(false, targetArch))));

	for (ModuleLocation m : overriddenCompiledModulesRemoved)
		expectedOverriddenCompiledModulesRemoved.append(replacePidDirectory(replaceKindDirectory(m.getCachedOverriddenCompiledModulePath(false, targetArch))));

	actualCompiledModulesAdded.sort();
	actualCompiledModulesModified.sort();
	actualCompiledModulesRemoved.sort();
	expectedCompiledModulesAdded.sort();
	expectedCompiledModulesModified.sort();
	expectedCompiledModulesRemoved.sort();
	actualOverriddenCompiledModulesAdded.sort();
	actualOverriddenCompiledModulesModified.sort();
	actualOverriddenCompiledModulesRemoved.sort();
	expectedOverriddenCompiledModulesAdded.sort();
	expectedOverriddenCompiledModulesModified.sort();
	expectedOverriddenCompiledModulesRemoved.sort();

	QCOMPARE(actualCompiledModulesAdded, expectedCompiledModulesAdded);
	QCOMPARE(actualCompiledModulesModified, expectedCompiledModulesModified);
	QCOMPARE(actualCompiledModulesRemoved, expectedCompiledModulesRemoved);
	QCOMPARE(actualOverriddenCompiledModulesAdded, expectedOverriddenCompiledModulesAdded);
	QCOMPARE(actualOverriddenCompiledModulesModified, expectedOverriddenCompiledModulesModified);
	QCOMPARE(actualOverriddenCompiledModulesRemoved, expectedOverriddenCompiledModulesRemoved);

	// Dylibs

	QStringList actualDylibsAdded;
	QStringList actualDylibsModified;
	QStringList actualDylibsRemoved;

	vector<string> moduleCachesWithMultipleDylibs;
	map<string, double> actualDylibs = listDylibsInAllModuleCaches(moduleCachesWithMultipleDylibs);
	QVERIFY2(moduleCachesWithMultipleDylibs.empty(), ("module caches contain more than 1 .dylib: " + VuoStringUtilities::join(moduleCachesWithMultipleDylibs, " ")).c_str());

	auto replaceRevisionSuffix = [](string dylibPath)
	{
		string dir, file, ext;
		VuoFileUtilities::splitPath(dylibPath, dir, file, ext);
		vector<string> fileParts = VuoStringUtilities::split(file, '-');
		if (fileParts.size() > 1)
			fileParts.back() = "*";
		string modifiedPath = dir + VuoStringUtilities::join(fileParts, '-') + "." + ext;
		return QString::fromStdString(modifiedPath);
	};

	for (auto i : actualDylibs)
	{
		auto sameRevisionnOfDylib = baselineDylibs.find(i.first);
		auto differentRevisionOfDylib = std::find_if(baselineDylibs.begin(), baselineDylibs.end(),
													 [i](pair<string, double> j) { return VuoModuleCache::areDifferentRevisionsOfSameDylib(i.first, j.first); });

		if (sameRevisionnOfDylib != baselineDylibs.end())
		{
			QVERIFY2(i.second == sameRevisionnOfDylib->second, (i.first + " was overwritten instead of a new .dylib being generated").c_str());
			baselineDylibs.erase(sameRevisionnOfDylib);
		}
		else if (differentRevisionOfDylib != baselineDylibs.end())
		{
			actualDylibsModified.append(replaceRevisionSuffix(i.first));
			baselineDylibs.erase(differentRevisionOfDylib);
		}
		else
		{
			actualDylibsAdded.append(replaceRevisionSuffix(i.first));
		}
	}

	for (auto i : baselineDylibs)
		actualCompiledModulesRemoved.append(replaceRevisionSuffix(i.first));

	QStringList expectedDylibsAdded;
	QStringList expectedDylibsModified;
	QStringList expectedDylibsRemoved;

	for (ModuleScope scope : dylibsAdded)
		expectedDylibsAdded.append(replaceRevisionSuffix(scope.getModuleCache()->getDylibPath()));

	for (ModuleScope scope : dylibsModified)
		expectedDylibsModified.append(replaceRevisionSuffix(scope.getModuleCache()->getDylibPath()));

	for (ModuleScope scope : dylibsRemoved)
		expectedDylibsRemoved.append(replaceRevisionSuffix(scope.getModuleCache()->getDylibPath()));

	actualDylibsAdded.sort();
	actualDylibsModified.sort();
	actualDylibsRemoved.sort();
	expectedDylibsAdded.sort();
	expectedDylibsModified.sort();
	expectedDylibsRemoved.sort();

	QCOMPARE(actualDylibsAdded, expectedDylibsAdded);
	QCOMPARE(actualDylibsModified, expectedDylibsModified);
	QCOMPARE(actualDylibsRemoved, expectedDylibsRemoved);

	// Manifests

	for (auto i : manifestShouldContain)
	{
		ModuleScope scope = i.first;
		VuoModuleCacheManifest manifest = scope.getManifest();

		for (string moduleKey : i.second)
			QVERIFY2(manifest.contains(moduleKey), string("should be in " + scope.getModuleCache()->getManifestPath() + ": " + moduleKey).c_str());
	}

	for (auto i : manifestShouldNotContain)
	{
		ModuleScope scope = i.first;
		VuoModuleCacheManifest manifest = scope.getManifest();

		if (i.second.size() == 1 && i.second.front() == "*")
			QVERIFY2(manifest.getContents().empty(), (scope.getModuleCache()->getManifestPath() + " should be empty").c_str());
		else
			for (string moduleKey : i.second)
				QVERIFY2(! manifest.contains(moduleKey), string("should not be in " + scope.getModuleCache()->getManifestPath() + ": " + moduleKey).c_str());
	}
}

void ModuleCachesDiff::setTarget(string target)
{
	targetArch = VuoCompiler::getTargetArch(target);
}
