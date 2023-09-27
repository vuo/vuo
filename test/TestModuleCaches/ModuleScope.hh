/**
 * @file
 * ModuleScope interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

/**
 * A level of scope at which modules are installed and cached, with functions for getting info specific to
 * that scope and specifying the composition location for the composition-family and composition scopes.
 */
class ModuleScope
{
public:
	enum Scope
	{
		System,
		User,
		CompositionFamily,
		Composition
	};

	ModuleScope(Scope scope, string compositionDirName = "");
	string getCompositionDirName();
	shared_ptr<VuoModuleCache> getModuleCache();
	string getInstalledModulesPath();
	VuoModuleCacheManifest getManifest();

	static string registerCompositionDirectory(string compositionDirName);
	static void deleteAllInstalledTestModules();

	friend bool operator<(const ModuleScope &lhs, const ModuleScope &rhs);

private:
	Scope scope;
	string compositionDirName;

	class KnownModuleCaches
	{
	public:
		KnownModuleCaches();
		map<Scope, map<string, shared_ptr<VuoModuleCache>>> moduleCaches;
	};

	static KnownModuleCaches * getKnownModuleCaches();
	static KnownModuleCaches *knownModuleCaches;

	class KnownModulesDirectories
	{
	public:
		KnownModulesDirectories();
		map<Scope, map<string, string>> modulesDirectories;
	};

	static KnownModulesDirectories * getKnownModulesDirectories();
	static KnownModulesDirectories *knownModulesDirectories;
};
