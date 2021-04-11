/**
 * @file
 * VuoCompilerModule implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoJsonUtilities.hh"
#include "VuoNodeClass.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a VuoCompilerModule associated with the given LLVM module,
 * as a pseudo compiler detail class of the given VuoModule.
 */
VuoCompilerModule::VuoCompilerModule(VuoModule *base, Module *module)
	: compatibleTargets(nullptr)
{
#if VUO_PRO
	init_Pro();
#endif
	this->base = base;
	this->module = module;
	this->moduleDetails = NULL;
	this->parser = NULL;
	this->builtIn = false;

	if (module)
	{
		module->setModuleIdentifier(getPseudoBase()->getModuleKey());
		parse();
	}
}

/**
 * Destructor.
 */
VuoCompilerModule::~VuoCompilerModule(void)
{
#if VUO_PRO
	fini_Pro();
#endif
	delete parser;
	json_object_put(moduleDetails);
}

/**
 * Instantiates a VuoCompilerModule (or child class) corresponding to the type of
 * VuoCompilerModule defined in the LLVM module. If no type of VuoCompilerModule is defined,
 * returns NULL.
 */
VuoCompilerModule * VuoCompilerModule::newModule(const string &moduleKey, Module *module, const string &modulePath)
{
	VuoCompilerModule *m = NULL;

	if (isModule(module, moduleKey))
	{
		VuoNodeClass *nodeClass = VuoCompilerNodeClass::newNodeClass(moduleKey, module);
		if (nodeClass)
			m = nodeClass->getCompiler();
		else
		{
			VuoCompilerType *type = VuoCompilerType::newType(moduleKey, module);
			if (type)
				m = type;
			else
				m = new VuoCompilerModule(new VuoModule(moduleKey), module);
		}

		m->modulePath = modulePath;
	}
	else if (moduleKey != "libmodule")
		VUserLog("Warning: No VuoModuleMetadata found in \"%s\" so I'm not going to load it.", moduleKey.c_str());

	return m;
}

/**
 * Returns true if the LLVM module contains certain global variables, indicating that it is
 * intended to be a VuoCompilerModule definition.
 */
bool VuoCompilerModule::isModule(Module *module, string moduleKey)
{
	return hasOriginalOrMangledGlobal("moduleDetails", module, moduleKey);
}

/**
 * Renames globals in the LLVM module that might conflict with other VuoCompilerModules,
 * and parses those globals.
 */
void VuoCompilerModule::parse(void)
{
	parser = new VuoCompilerBitcodeParser(module);
	renameGlobalVarsAndFuncs();
	parseMetadata();
}

/**
 * Parses the metadata of this VuoCompilerModule (name, description, ...) from the LLVM module.
 */
void VuoCompilerModule::parseMetadata(void)
{
	string moduleDetailsStr = parser->getGlobalString( nameForGlobal("moduleDetails") );
	json_object_put(moduleDetails);
	moduleDetails = json_tokener_parse(moduleDetailsStr.c_str());

	if (! moduleDetails)
	{
		VUserLog("Error: Couldn't parse VuoModuleMetadata as JSON: %s", moduleDetailsStr.c_str());
		return;
	}

	base->setDefaultTitle(VuoJsonUtilities::parseString(moduleDetails, "title"));
	base->setDescription(VuoJsonUtilities::parseString(moduleDetails, "description"));
	base->setVersion(VuoJsonUtilities::parseString(moduleDetails, "version"));
	base->setKeywords(VuoJsonUtilities::parseArrayOfStrings(moduleDetails, "keywords"));
	vector<string> dependenciesVector = VuoJsonUtilities::parseArrayOfStrings(moduleDetails, "dependencies");
	dependencies = set<string>(dependenciesVector.begin(), dependenciesVector.end());
	compatibleTargets = parseCompatibility(moduleDetails, "compatibility");
}

/**
 * Parses the target set value for @a key in the top level of the JSON object.
 *
 * If no such value is found, returns an unrestricted target set.
 */
VuoCompilerCompatibility VuoCompilerModule::parseCompatibility(json_object *o, string key)
{
	json_object *compatibilityObject = nullptr;
	if (json_object_object_get_ex(o, key.c_str(), &compatibilityObject))
		return VuoCompilerCompatibility(compatibilityObject);

	return VuoCompilerCompatibility(nullptr);
}

/**
 * Returns a list of global variables and functions that may be defined in multiple modules,
 * and thus need to be renamed.
 */
set<string> VuoCompilerModule::globalsToRename(void)
{
	set<string> globals;
	globals.insert("moduleDetails");
	return globals;
}

/**
 * Returns the mangled name for a function or global variable.
 */
string VuoCompilerModule::nameForGlobal(string genericGlobalVarOrFuncName)
{
	return nameForGlobal(genericGlobalVarOrFuncName, base->getModuleKey());
}

/**
 * Returns the mangled name for a function or global variable.
 */
string VuoCompilerModule::nameForGlobal(string nameBeforeCompilation, string moduleKey)
{
	return VuoStringUtilities::prefixSymbolName(nameBeforeCompilation, moduleKey);
}

/**
 * Returns true if the module contains a function or global variable with name @c nameBeforeCompilation
 * or with the corresponding mangled name.
 */
bool VuoCompilerModule::hasOriginalOrMangledGlobal(string nameBeforeCompilation, Module *module, string moduleKey)
{
	string nameAfterCompilation = nameForGlobal(nameBeforeCompilation, moduleKey);
	return (module->getNamedValue(nameBeforeCompilation) != NULL || module->getNamedValue(nameAfterCompilation) != NULL);
}

/**
 * Renames the global variables and functions within the LLVM module so that they are
 * unique to this VuoCompilerModule.
 */
void VuoCompilerModule::renameGlobalVarsAndFuncs(void)
{
	set<string> globalsAndFuncsToRename = globalsToRename();

	Module::GlobalListType& globals = module->getGlobalList();

	// Iterate through global variables
	for (Module::GlobalListType::iterator i = globals.begin(), e = globals.end(); i != e; ++i) {
		string curGlobalVarName = i->getName();
		// If current global variable is in the list of those to rename, do so
		if (globalsAndFuncsToRename.find(curGlobalVarName) != globalsAndFuncsToRename.end()) {
			string newGlobalVarName = nameForGlobal(curGlobalVarName);
			i->setName(newGlobalVarName);
		}

	}

	Module::FunctionListType& functions = module->getFunctionList();

	// Iterate through functions
	for (Module::FunctionListType::iterator i = functions.begin(), e = functions.end(); i != e; ++i) {
		string curFuncName = i->getName();
		// If current function is in the list of those to rename, do so
		if (globalsAndFuncsToRename.find(curFuncName) != globalsAndFuncsToRename.end()) {
			string newFuncName = nameForGlobal(curFuncName);
			i->setName(newFuncName);
		}

	}
}

/**
 * Copies the function's header into the LLVM module, if it's not already there.
 * Use this when an LLVM module needs to call a function defined in another LLVM module.
 */
Function * VuoCompilerModule::declareFunctionInModule(Module *module, Function *functionSrc)
{
	Function *functionDst = module->getFunction(functionSrc->getName());
	if (! functionDst)
	{
		functionDst = Function::Create(functionSrc->getFunctionType(),
									   GlobalValue::ExternalLinkage,
									   functionSrc->getName(),
									   module);
		functionDst->setAttributes( functionSrc->getAttributes() );
	}
	return functionDst;
}

/**
 * Returns a list of this VuoCompilerModule's dependencies.
 */
set<string> VuoCompilerModule::getDependencies(void)
{
	return dependencies;
}

/**
 * Returns the name that would represent this VuoCompilerModule in another VuoCompilerModule's
 * list of dependencies.
 */
string VuoCompilerModule::getDependencyName(void)
{
	return base->getModuleKey();
}

/**
 * Returns the set of targets (operating system versions) with which this module is compatible.
 */
VuoCompilerCompatibility VuoCompilerModule::getCompatibleTargets(void)
{
	return compatibleTargets;
}

/**
 * Returns this VuoCompilerModule's LLVM module, which other LLVM modules can link to.
 */
Module * VuoCompilerModule::getModule(void)
{
	return module;
}

/**
 * Returns the (pseudo) base for this (pseudo) compiler detail class.
 */
VuoModule * VuoCompilerModule::getPseudoBase(void)
{
	return base;
}

/**
 * Returns true if this module is one of the built-in modules distributed with Vuo.
 */
bool VuoCompilerModule::isBuiltIn(void)
{
	return builtIn;
}

/**
 * Sets whether this module is one of the built-in modules distributed with Vuo.
 */
void VuoCompilerModule::setBuiltIn(bool builtIn)
{
	this->builtIn = builtIn;
}

/**
 * Returns the file from which the LLVM module was loaded, or an empty string if the module
 * was contained in an archive or generated by the compiler.
 */
string VuoCompilerModule::getModulePath(void)
{
	return modulePath;
}
