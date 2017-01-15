/**
 * @file
 * VuoCompilerModule interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERMODULE_HH
#define VUOCOMPILERMODULE_HH

#include "VuoCompilerTargetSet.hh"

class VuoCompilerBitcodeParser;
class VuoCompilerModule;
class VuoModule;

/**
 * A node class or type defined in an LLVM module.
 *
 * This class would be the compiler detail class for @c VuoModule, except that the inheritance doesn't work out.
 */
class VuoCompilerModule
{
private:
	VuoCompilerTargetSet compatibleTargets;  ///< The set of targets with which this module is compatible.
	bool builtIn;

	static bool isModule(Module *module, string moduleKey);

protected:
	struct json_object *moduleDetails;  ///< This module's metadata, found in the argument to @c VuoModuleMetadata in the module definition.
	set<string> dependencies;  ///< The dependencies found in this module's metadata
	Module *module;  ///< The LLVM module that defines this node class or type
	VuoModule *base;  ///< The (pseudo) base for this (pseudo) compiler detail class
	VuoCompilerBitcodeParser *parser;  ///< The parser of the LLVM module
	bool isPremium;  ///< A boolean indicating whether this module contains premium content

	VuoCompilerModule(VuoModule *base, Module *module);

	virtual void parse(void);
	virtual void parseMetadata(void);
	static string parseString(json_object *o, string key, bool *foundValue=NULL);
	static int parseInt(json_object *o, string key, bool *foundValue=NULL);
	static bool parseBool(json_object *o, string key, bool *foundValue=NULL);
	static vector<string> parseArrayOfStrings(json_object *o, string key);
	virtual set<string> globalsToRename(void);
	string nameForGlobal(string genericGlobalVarOrFuncName);
	static string nameForGlobal(string nameBeforeCompilation, string moduleKey);
	static bool hasOriginalOrMangledGlobal(string nameBeforeCompilation, Module *module, string moduleKey);
	void renameGlobalVarsAndFuncs(void);

	friend class TestVuoCompilerModule;

public:
	static VuoCompilerModule * newModule(string moduleKey, Module *module);
	virtual ~VuoCompilerModule(void);

	VuoCompilerTargetSet parseTargetSet(json_object *o, string key);
	VuoCompilerTargetSet::MacVersion parseMacVersion(string version);
	static Function * declareFunctionInModule(Module *module, Function *function);
	virtual set<string> getDependencies(void);
	virtual string getDependencyName(void);
	VuoCompilerTargetSet getCompatibleTargets(void);
	Module * getModule(void);
	VuoModule * getPseudoBase(void);
	bool getPremium(void);
	void setPremium(bool premium);
	bool isBuiltIn(void);
	void setBuiltIn(bool builtIn);
};

#endif // VUOCOMPILERMODULE_HH
