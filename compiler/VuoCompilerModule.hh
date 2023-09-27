/**
 * @file
 * VuoCompilerModule interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerCompatibility.hh"

class VuoCompilerBitcodeParser;
class VuoMakeDependencies;
class VuoModule;

/**
 * A node class or type defined in an LLVM module.
 *
 * This class would be the compiler detail class for @c VuoModule, except that the inheritance doesn't work out.
 */
class VuoCompilerModule
{
private:
	VuoModule *base;  ///< The (pseudo) base for this (pseudo) compiler detail class
	bool builtIn;  ///< True if this is one of the modules built-in to Vuo.
	string modulePath;  ///< The file from which the LLVM module was loaded, if any.

	static bool isModule(Module *module, string moduleKey);

protected:
	struct json_object *moduleDetails;  ///< This module's metadata, found in the argument to @c VuoModuleMetadata in the module definition.
	set<string> dependencies;  ///< The modules and libraries needed to compile a composition containing this module.
	VuoCompilerCompatibility compatibleTargets;  ///< The set of targets with which this module is compatible.
	Module *module;  ///< The LLVM module that defines this node class or type
	VuoCompilerBitcodeParser *parser;  ///< The parser of the LLVM module
	string sourcePath;  ///< The file containing the module's source code, if known.
	string sourceCode;  ///< The module's source code, if known.
	shared_ptr<VuoMakeDependencies> makeDependencies;  ///< The source files needed to compile this module.

	VuoCompilerModule(VuoModule *base, Module *module);

	static bool isSpecializedModule(Module *module, string moduleKey);
	virtual void parse(void);
	virtual void parseMetadata(void);
	virtual set<string> globalsToRename(void);
	string nameForGlobal(string genericGlobalVarOrFuncName);
	static string nameForGlobal(string nameBeforeCompilation, string moduleKey);
	static bool hasOriginalOrMangledGlobal(string nameBeforeCompilation, Module *module, string moduleKey);
	void renameGlobalVarsAndFuncs(void);

	map<string, string> defaultSpecializedForGenericTypeName;  ///< For generic modules: the types to use when creating an unspecialized instance.
	map<string, vector<string> > compatibleSpecializedForGenericTypeName;  ///< For generic modules: the types to which each generic type is allowed to be specialized.

	friend class TestVuoCompilerModule;
	friend class TestModules;

public:
	static VuoCompilerModule * newModule(const string &moduleKey, Module *module, const string &modulePath, const VuoCompilerCompatibility &moduleCompatibility);
	virtual ~VuoCompilerModule(void);

	static VuoCompilerCompatibility parseCompatibility(json_object *o, string key);
	static void parseGenericTypes(json_object *moduleDetails, vector<string> &genericTypeNames, map<string, string> &defaultSpecializedForGenericTypeName, map<string, vector<string> > &compatibleSpecializedForGenericTypeName);
	static Function * declareFunctionInModule(Module *module, Function *function);
	virtual set<string> getDependencies(void);
	virtual string getDependencyName(void);
	virtual string getDependencyPath(void);
	virtual string getFileName(void);
	static string getFileNameForModuleKey(const string &moduleKey);
	static bool isModuleKeyMangledInFileName(const string &fileName);
	VuoCompilerCompatibility getCompatibleTargets(void);
	Module * getModule(void);
	VuoModule * getPseudoBase(void);
	bool isBuiltIn(void);
	void setBuiltIn(bool builtIn);
	string getModulePath(void);
	void setSourcePath(const string &sourcePath);
	string getSourcePath(void);
	void setSourceCode(const string &sourceCode);
	string getSourceCode(void);
	shared_ptr<VuoMakeDependencies> getMakeDependencies(void);

private:
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
	void *p;
#pragma clang diagnostic pop
#if VUO_PRO
#include "../compiler/pro/VuoCompilerModule_Pro.hh"
#endif
};
