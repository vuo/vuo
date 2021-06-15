/**
 * @file
 * VuoCompilerGroup implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerGroup.hh"

#include "VuoCompiler.hh"
#include "VuoCompilerComposition.hh"
#include "VuoCompilerException.hh"
#include "VuoCompilerIssue.hh"
#include "VuoException.hh"
#include "VuoFileUtilitiesCocoa.hh"

/**
 * Creates a compiler for each architecture supported by this Vuo build.
 */
VuoCompilerGroup * VuoCompilerGroup::compilersForAllDeploymentArchitectures(const string &compositionPath)
{
	return compilersForAllCompatibleDeploymentArchitectures(VuoCompilerCompatibility::compatibilityWithAnySystem(), compositionPath);
}

/**
 * Creates a compiler for each architecture supported by this Vuo build and the composition.
 *
 * The OS version of each created compiler's target is the minimum compatible with the composition.
 *
 * If an architecture is not compatible with the composition, then the compiler for that architecture is omitted
 * from the returned group.
 */
VuoCompilerGroup * VuoCompilerGroup::compilersForAllCompatibleDeploymentArchitectures(const string &compositionString, const string &compositionPath)
{
	VuoCompiler nominalCompiler(compositionPath);
	nominalCompiler.setLoadAllModules(false);

	return VuoCompilerGroup::compilersForAllCompatibleDeploymentArchitectures(compositionString, &nominalCompiler);
}

/**
 * Creates a compiler for each architecture supported by this Vuo build and the composition.
 *
 * The OS version of each created compiler's target is the minimum compatible with the composition.
 *
 * If an architecture is not compatible with the composition, then the compiler for that architecture is omitted
 * from the returned group.
 */
VuoCompilerGroup * VuoCompilerGroup::compilersForAllCompatibleDeploymentArchitectures(const string &compositionString, VuoCompiler *nominalCompiler)
{
	VuoCompilerComposition *nominalComposition = VuoCompilerComposition::newCompositionFromGraphvizDeclaration(compositionString, nominalCompiler);
	set<string> dependencies = nominalCompiler->getDependenciesForComposition(nominalComposition);
	VuoCompilerCompatibility compatibility = nominalCompiler->getCompatibilityOfDependencies(dependencies);

	string compositionPath = nominalCompiler->getCompositionLocalPath() + "/unused";
	return VuoCompilerGroup::compilersForAllCompatibleDeploymentArchitectures(compatibility, compositionPath);
}

/**
 * Creates a compiler for each architecture supported by this Vuo build, constrained by @a compatibility.
 *
 * The OS version of each created compiler's target is the minimum allowed by @a compatibility.
 *
 * If an architecture is not allowed by @a compatibility, then the compiler for that architecture is omitted
 * from the returned group.
 */
VuoCompilerGroup * VuoCompilerGroup::compilersForAllCompatibleDeploymentArchitectures(VuoCompilerCompatibility compatibility, const string &compositionPath)
{
	vector<vector<string>> defaultTargets = {
#ifdef VUO_TARGET_ARCHITECTURE_X86_64
		{"macos", "x86_64", "10.10", "apple-macosx"},
#endif
#ifdef VUO_TARGET_ARCHITECTURE_ARM64
		{"macos", "arm64", "10.10", "apple-macosx"},
#endif
	};

	if (defaultTargets.empty())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::IssueType::Error, "setting up compiler", "",
							   "Vuo build misconfigured",
							   "This build of Vuo doesn't support any architectures.");
		throw VuoCompilerException(issue);
	}

	vector<VuoCompiler *> compilers;
	for (vector<string> defaultTarget : defaultTargets)
	{
		json_object *platformArchJson = json_object_new_object();

		json_object *platformVal = json_object_new_object();
		json_object_object_add(platformArchJson, defaultTarget[0].c_str(), platformVal);

		json_object *archVal = json_object_new_string(defaultTarget[1].c_str());
		json_object *archArray = json_object_new_array_ext(1);
		json_object_array_put_idx(archArray, 0, archVal);
		json_object_object_add(platformVal, "arch", archArray);

		VuoCompilerCompatibility platformArchCompatibility(platformArchJson);

		VuoCompilerCompatibility bothCompatibility = platformArchCompatibility.intersection(compatibility);

		if (bothCompatibility.isCompatibleWithPlatform(defaultTarget[0]))
		{
			string minVersion = bothCompatibility.getMinVersionOnPlatform(defaultTarget[0]);
			if (minVersion.empty())
				minVersion = defaultTarget[2];

			string target = defaultTarget[1] + "-" + defaultTarget[3] + minVersion + ".0";

			VuoCompiler *compiler = new VuoCompiler(compositionPath, target);
			compilers.push_back(compiler);
		}
	}

	return new VuoCompilerGroup(compilers, compatibility);
}

/**
 * Returns a compiler that matches the current machine's architecture.
 */
VuoCompiler * VuoCompilerGroup::compilerForCurrentArchitecture(void)
{
	string arch = VuoFileUtilitiesCocoa_getArchitecture();

	vector<VuoCompiler *> matchingCompilers;
	std::copy_if(compilers.begin(), compilers.end(),
				 std::back_inserter(matchingCompilers),
				 [&arch](VuoCompiler *compiler){ return compiler->getArch() == arch; });

	if (matchingCompilers.empty())
	{
		VuoCompilerIssue issue(VuoCompilerIssue::IssueType::Error, "setting up compiler", "",
							   "Unsupported architecture",
							   "This build of Vuo doesn't support the current system's architecture (" + arch + ")");
		throw VuoCompilerException(issue);
	}

	return matchingCompilers.front();
}

/**
 * Calls the callback for each compiler in the group.
 */
void VuoCompilerGroup::doForEach(std::function<void(VuoCompiler *)> callback)
{
	for (auto compiler : compilers)
		callback(compiler);
}

/**
 * Returns the union of all targets of this group of compilers.
 */
VuoCompilerCompatibility VuoCompilerGroup::getCompatibleTargets(void)
{
	return compatibleTargets;
}

/**
 * Constructs a group consisting of @a compilers. This class takes ownership of the VuoCompiler objects.
 */
VuoCompilerGroup::VuoCompilerGroup(const vector<VuoCompiler *> &compilers, const VuoCompilerCompatibility &compatibleTargets) :
	compilers(compilers), compatibleTargets(compatibleTargets)
{
}

/**
 * Destroys all of the compilers in the group.
 */
VuoCompilerGroup::~VuoCompilerGroup(void)
{
	for (auto compiler : compilers)
		delete compiler;
}
