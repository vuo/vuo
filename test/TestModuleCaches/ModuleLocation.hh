/**
 * @file
 * ModuleLocation interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "ModuleScope.hh"

/**
 * An abstraction of the location at which a module is installed, allowing the caller to specify
 * the module's location abstractly by scope and get back concrete file paths for different forms
 * of the module (source file, compiled file).
 */
class ModuleLocation
{
public:
	ModuleLocation(ModuleScope scope, string moduleKey, string extension);
	ModuleScope getScope();
	string getSourcePath();
	string getInstalledModulePath();
	string getCachedCompiledModulePath(bool isGenerated, string targetArch);
	string getCachedOverriddenCompiledModulePath(bool isGenerated, string targetArch);
	void setInstalledModuleKey(string installedModuleKey);

private:
	ModuleScope scope;
	string moduleKey;
	string installedFileName;
	string extension;
};
