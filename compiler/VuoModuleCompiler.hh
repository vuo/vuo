/**
 * @file
 * VuoModuleCompiler interface.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoFileUtilities.hh"
class VuoCompilerIssues;
class VuoCompilerType;

/**
 * Base class for compiling node classes from various source formats.
 */
class VuoModuleCompiler
{
public:
	static VuoModuleCompiler *newModuleCompiler(string type, const string &moduleKey, VuoFileUtilities::File *sourceFile);
	virtual ~VuoModuleCompiler(void);

	/**
	 * Compiles the source file to LLVM bitcode that can be loaded as a Vuo node class.
	 *
	 * If there are errors, they are reported in @a issues and the return value is null.
	 */
	virtual Module *compile(std::function<VuoCompilerType *(const string &)> getVuoType, dispatch_queue_t llvmQueue, VuoCompilerIssues *issues) = 0;

	/**
	 * When compiling, @a sourceCode will be used instead of the contents of the source file.
	 *
	 * Optionally, @a sourceFile specifies which file to override when the module has multiple source files.
	 */
	virtual void overrideSourceCode(const string &sourceCode, VuoFileUtilities::File *sourceFile) = 0;

protected:
	/**
	 * A method that creates an instance of a subclass of VuoModuleCompiler.
	 */
	typedef VuoModuleCompiler *(*Factory)(const string &moduleKey, VuoFileUtilities::File *sourceFile);

	static void registerModuleCompiler(string type, Factory factory);

private:
	static void __attribute__((constructor)) init();

	static map<string, Factory> *factories;
};
