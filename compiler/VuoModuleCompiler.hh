/**
 * @file
 * VuoModuleCompiler interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoModuleCompilerResults.hh"
#include "VuoModuleCompilerSettings.hh"
class VuoCompilerIssues;
class VuoCompilerType;

/**
 * Base class for compiling node classes from various source formats.
 */
class VuoModuleCompiler
{
public:
	static VuoModuleCompiler * newModuleCompiler(const string &type, const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings, std::function<VuoCompilerType *(const string &)> getVuoType);
	virtual ~VuoModuleCompiler(void);

	/**
	 * Compiles the source file to LLVM bitcode that can be loaded as a Vuo node class.
	 *
	 * The resulting LLVM module and any other generated artifacts are placed in the returned value.
	 * If compilation fails, the return value's `module` field is null and errors are reported in @a issues.
	 */
	virtual VuoModuleCompilerResults compile(dispatch_queue_t llvmQueue, VuoCompilerIssues *issues) = 0;

	virtual string generateHeader(VuoCompilerIssues *issues);

	/**
	 * When compiling, @a sourceCode will be used instead of the contents of the source file.
	 *
	 * Optionally, @a sourceFile specifies which file to override when the module has multiple source files.
	 */
	virtual void overrideSourceCode(const string &sourceCode, const string &sourcePath) = 0;

protected:
	/**
	 * A method that creates an instance of a subclass of VuoModuleCompiler.
	 */
	typedef VuoModuleCompiler *(*Factory)(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);

	static void registerModuleCompiler(string type, Factory factory);
	VuoModuleCompiler(const string &moduleKey, const string &sourcePath, const VuoModuleCompilerSettings &settings);
	VuoCompilerType * lookUpVuoType(const string &typeName);

	string moduleKey;  ///< The module key of the module to compile.
	string sourcePath;  ///< The path of the primary source file to compile.
	VuoModuleCompilerSettings settings;  ///< Settings to use when compiling the module (specific to the type of module compiler).
	map<string, VuoCompilerType *> vuoTypes;   ///< Cached results of lookUpVuoType(), stored so they can be used on `llvmQueue`.

private:
	static void loadAllModuleCompilers(void);

	static map<string, Factory> *factories;
	std::function<VuoCompilerType *(const string &)> getVuoType;
};
