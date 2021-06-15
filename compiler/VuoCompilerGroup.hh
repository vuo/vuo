/**
 * @file
 * VuoCompilerGroup interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

class VuoCompiler;

#include "VuoCompilerCompatibility.hh"

/**
 * A set of compilers, each having a different target, to be used to compile a composition or other source to multiple targets.
 */
class VuoCompilerGroup
{
public:
	static VuoCompilerGroup * compilersForAllDeploymentArchitectures(const string &compositionPath = "");
	static VuoCompilerGroup * compilersForAllCompatibleDeploymentArchitectures(const string &compositionString, const string &compositionPath = "");
	static VuoCompilerGroup * compilersForAllCompatibleDeploymentArchitectures(const string &compositionString, VuoCompiler *nominalCompiler);
	VuoCompiler * compilerForCurrentArchitecture(void);
	void doForEach(std::function<void(VuoCompiler *)> callback);
	VuoCompilerCompatibility getCompatibleTargets(void);
	~VuoCompilerGroup(void);

private:
	static VuoCompilerGroup * compilersForAllCompatibleDeploymentArchitectures(VuoCompilerCompatibility compatibility, const string &compositionPath = "");
	explicit VuoCompilerGroup(const vector<VuoCompiler *> &compilers, const VuoCompilerCompatibility &compatibleTargets);

	vector<VuoCompiler *> compilers;
	VuoCompilerCompatibility compatibleTargets;
};
