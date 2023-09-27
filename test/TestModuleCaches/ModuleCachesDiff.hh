/**
 * @file
 * ModuleCachesDiff interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "ModuleLocation.hh"

/**
 * Specification of the expected difference in the contents of the module caches before and after
 * a change to the installed modules.
 */
class ModuleCachesDiff
{
public:
	vector<ModuleLocation> compiledModulesAdded;
	vector<ModuleLocation> compiledModulesModified;
	vector<ModuleLocation> compiledModulesRemoved;
	vector<ModuleLocation> overriddenCompiledModulesAdded;
	vector<ModuleLocation> overriddenCompiledModulesModified;
	vector<ModuleLocation> overriddenCompiledModulesRemoved;
	vector<ModuleScope> dylibsAdded;
	vector<ModuleScope> dylibsModified;
	vector<ModuleScope> dylibsRemoved;
	map<ModuleScope, vector<string>> manifestShouldContain;
	map<ModuleScope, vector<string>> manifestShouldNotContain;

	void recordBaseline();
	void check();

	static void setTarget(string target);

private:
	map<string, double> listCompiledModulesInAllModuleCaches();
	map<string, double> listDylibsInAllModuleCaches(vector<string> &moduleCachesWithMultipleDylibs);

	map<string, double> baselineCompiledModules;
	map<string, double> baselineDylibs;

	static string targetArch;
};
