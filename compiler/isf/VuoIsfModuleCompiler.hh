/**
 * @file
 * VuoIsfModuleCompiler interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
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
	VuoModuleCompilerResults compile(dispatch_queue_t llvmQueue, VuoCompilerIssues *issues);
	void overrideSourceCode(const string &sourceCode, const string &sourcePath);

private:
	static void __attribute__((constructor)) init();
	static VuoModuleCompiler * newModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);
	VuoIsfModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);
	void generateMetadata(Module *module);
	void generateNodeInstanceInitFunction(Module *module, VuoCompilerConstantsCache *constantsCache, map<string, VuoCompilerType *> vuoTypes, VuoCompilerIssues *issues);
	void generateNodeInstanceEventFunction(Module *module, VuoCompilerConstantsCache *constantsCache, map<string, VuoCompilerType *> vuoTypes, VuoCompilerIssues *issues);
	bool isTypeFound(VuoCompilerType *type, VuoCompilerIssues *issues);
	Type * getFunctionParameterType(VuoCompilerType *type, VuoCompilerIssues *issues);

	VuoShaderFile *shaderFile;
	string sourceCode;
};
