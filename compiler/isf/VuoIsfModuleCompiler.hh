/**
 * @file
 * VuoIsfModuleCompiler interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include <Vuo/Vuo.h>

/**
 * Responsible for compiling node classes implemented in ISF (image generators/filters and object renderers/filters).
 */
class VuoIsfModuleCompiler : public VuoModuleCompiler
{
public:
	Module * compile(std::function<VuoCompilerType *(const string &)> getVuoType, dispatch_queue_t llvmQueue, VuoCompilerIssues *issues);
	void overrideSourceCode(const string &sourceCode, VuoFileUtilities::File *sourceFile);

private:
	static void __attribute__((constructor)) init();
	static VuoModuleCompiler *newModuleCompiler(const string &moduleKey, VuoFileUtilities::File *sourceFile);
	VuoIsfModuleCompiler(const string &moduleKey, VuoFileUtilities::File *sourceFile);
	void generateMetadata(Module *module);
	void generateNodeInstanceInitFunction(Module *module, VuoCompilerConstantsCache *constantsCache, map<string, VuoCompilerType *> vuoTypes);
	void generateNodeInstanceEventFunction(Module *module, VuoCompilerConstantsCache *constantsCache, map<string, VuoCompilerType *> vuoTypes);

	string moduleKey;
	VuoShaderFile *shaderFile;
};
