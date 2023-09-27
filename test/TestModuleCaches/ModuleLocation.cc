/**
 * @file
 * ModuleLocation implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "ModuleLocation.hh"

ModuleLocation::ModuleLocation(ModuleScope scope, string moduleKey, string extension) :
	scope(scope),
	moduleKey(moduleKey),
	extension(extension)
{
	setInstalledModuleKey(moduleKey);
}

ModuleScope ModuleLocation::getScope()
{
	return scope;
}

string ModuleLocation::getSourcePath()
{
	string dir = extension == "vuonode" ?
					 BINARY_DIR "/test/TestModuleCaches" :
					 VUO_SOURCE_DIR "/test/TestModuleCaches/node-TestModuleCaches";
	return dir + "/" + moduleKey + "." + extension;
}

string ModuleLocation::getInstalledModulePath()
{
	string installedModuleKey = VuoCompiler::getModuleKeyForPath(installedFileName);
	return scope.getInstalledModulesPath() + "/" + installedModuleKey + "." + extension;
}

string ModuleLocation::getCachedCompiledModulePath(bool isGenerated, string targetArch)
{
	return scope.getModuleCache()->getCompiledModulesPath(isGenerated, targetArch) + "/" + installedFileName;
}

string ModuleLocation::getCachedOverriddenCompiledModulePath(bool isGenerated, string targetArch)
{
	return scope.getModuleCache()->getOverriddenCompiledModulesPath(isGenerated, targetArch) + "/" + installedFileName;
}

void ModuleLocation::setInstalledModuleKey(string installedModuleKey)
{
	this->installedFileName = VuoCompilerModule::getFileNameForModuleKey(installedModuleKey);
}
