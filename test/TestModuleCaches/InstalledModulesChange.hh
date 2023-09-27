/**
 * @file
 * InstalledModulesChange interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "ModuleLocation.hh"
class TestCompilerDelegate;

/**
 * Specification of a set of changes (additions/modifications/deletions) to be made to the
 * source files and modules installed in User Modules and composition-local Modules directories.
 */
class InstalledModulesChange
{
public:
	vector<ModuleLocation> modulesToInstall;
	vector<ModuleLocation> modulesToModify;
	vector<ModuleLocation> modulesToDelete;
	vector<ModuleLocation> modulesToOverride;
	vector<ModuleLocation> modulesToRevert;

	void doChange(map<string, VuoCompiler *> compilerForCompositionDirName, map<string, TestCompilerDelegate *> delegateForCompositionDirName);
};
