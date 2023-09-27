/**
 * @file
 * InstalledModulesChange implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "InstalledModulesChange.hh"
#include "TestCompositionExecution.hh"
#include <fstream>

void InstalledModulesChange::doChange(map<string, VuoCompiler *> compilerForCompositionDirName,
									  map<string, TestCompilerDelegate *> delegateForCompositionDirName)
{
	auto getCompiler = [&compilerForCompositionDirName] (ModuleLocation m) -> VuoCompiler *
	{
		VuoCompiler *compiler = compilerForCompositionDirName[m.getScope().getCompositionDirName()];
		if (compiler)
			return compiler;
		else if (m.getScope().getCompositionDirName().empty() && compilerForCompositionDirName.size() == 1)
			return compilerForCompositionDirName.begin()->second;
		else
			return nullptr;
	};

	auto getDelegate = [&delegateForCompositionDirName] (ModuleLocation m) -> TestCompilerDelegate *
	{
		TestCompilerDelegate *delegate = delegateForCompositionDirName[m.getScope().getCompositionDirName()];
		if (delegate)
			return delegate;
		else if (m.getScope().getCompositionDirName().empty() && delegateForCompositionDirName.size() == 1)
			return delegateForCompositionDirName.begin()->second;
		else
			return nullptr;
	};

	for (ModuleLocation m : modulesToInstall)
		getDelegate(m)->installModule(m.getSourcePath(), m.getInstalledModulePath());

	for (ModuleLocation m : modulesToModify)
		getDelegate(m)->installModuleWithSuperficialChange(m.getSourcePath(), m.getInstalledModulePath());

	for (ModuleLocation m : modulesToDelete)
		getDelegate(m)->uninstallModule(m.getInstalledModulePath());

	for (ModuleLocation m : modulesToOverride)
		getDelegate(m)->overrideModuleWithSuperficialChange(m.getInstalledModulePath(), getCompiler(m));

	for (ModuleLocation m : modulesToRevert)
		getDelegate(m)->revertOverriddenModule(m.getInstalledModulePath(), getCompiler(m));
}
