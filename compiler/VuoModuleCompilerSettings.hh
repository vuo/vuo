/**
 * @file
 * VuoModuleCompilerSettings interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoNodeSet.hh"

/**
 * A collection of settings that may be used when compiling a module, depending on the type of VuoModuleCompiler.
 */
class VuoModuleCompilerSettings
{
public:
	VuoModuleCompilerSettings(void);

	map<string, string> typeNameReplacements;  ///< A mapping of generic types to the types they should be specialized to.
	VuoNodeSet *nodeSet;  ///< The node set that this module belongs to.
	string vuoFrameworkPath;  ///< The path of the Vuo.framework used by the current process, or a partially built Vuo.framework in which to find header files.
	string macOSSDKPath;  ///< Where to look for macOS system files when compiling the module, if other than `/`.
	vector<string> headerSearchPaths;  ///< Where to look for header files when compiling the module.
	string target;  ///< The target triple that the module should be compiled for.
	bool isVerbose;  ///< Whether to output extra messages when compiling the module.
};
