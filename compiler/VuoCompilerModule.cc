/**
 * @file
 * VuoCompilerModule implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdocumentation"
#include <json-c/json.h>
#pragma clang diagnostic pop

#include "VuoCompilerModule.hh"
#include "VuoCompilerBitcodeParser.hh"
#include "VuoCompilerNodeClass.hh"
#include "VuoCompilerType.hh"
#include "VuoStringUtilities.hh"

/**
 * Creates a VuoCompilerModule associated with the given LLVM module,
 * as a pseudo compiler detail class of the given VuoModule.
 */
VuoCompilerModule::VuoCompilerModule(VuoModule *base, Module *module)
{
	this->base = base;
	this->module = module;
	this->moduleDetails = NULL;
	this->parser = NULL;
	this->isPremium = false;
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
	delete parser;
	json_object_put(moduleDetails);
}

/**
 * Instantiates a VuoCompilerModule (or child class) corresponding to the type of
 * VuoCompilerModule defined in the LLVM module. If no type of VuoCompilerModule is defined,
 * returns NULL.
 */
VuoCompilerModule * VuoCompilerModule::newModule(string moduleKey, Module *module)
{
	if (isModule(module, moduleKey))
	{
		VuoNodeClass *nodeClass = VuoCompilerNodeClass::newNodeClass(moduleKey, module);
		if (nodeClass)
			return nodeClass->getCompiler();

		VuoCompilerType *type = VuoCompilerType::newType(moduleKey, module);
		if (type)
			return type;

		return new VuoCompilerModule(new VuoModule(moduleKey), module);
	}
	return NULL;
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

	base->setDefaultTitle( parseString(moduleDetails, "title") );
	base->setDescription( parseString(moduleDetails, "description") );
	base->setVersion( parseString(moduleDetails, "version") );
	base->setKeywords( parseArrayOfStrings(moduleDetails, "keywords") );
	vector<string> dependenciesVector = parseArrayOfStrings(moduleDetails, "dependencies");
	dependencies = set<string>(dependenciesVector.begin(), dependenciesVector.end());
	compatibleTargets = parseTargetSet(moduleDetails, "compatibleOperatingSystems");
}

/**
 * Parses the string value for @c key in the top level of the JSON object.
 *
 * If no such value is found, returns an empty string.
 */
string VuoCompilerModule::parseString(json_object *o, string key, bool *foundValue)
{
	string s;
	if (foundValue)
		*foundValue = false;

	json_object *stringObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &stringObject))
	{
		if (json_object_get_type(stringObject) == json_type_string)
		{
			s = json_object_get_string(stringObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return s;
}

/**
 * Parses the integer value for @c key in the top level of the JSON object.
 *
 * If no such value is found, returns 0.
 */
int VuoCompilerModule::parseInt(json_object *o, string key, bool *foundValue)
{
	int i = 0;
	if (foundValue)
		*foundValue = false;

	json_object *intObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &intObject))
	{
		if (json_object_get_type(intObject) == json_type_int)
		{
			i = json_object_get_int(intObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return i;
}

/**
 * Parses the boolean value for @c key in the top level of the JSON object.
 *
 * If no such value is found, returns false.
 */
bool VuoCompilerModule::parseBool(json_object *o, string key, bool *foundValue)
{
	bool b = 0;
	if (foundValue)
		*foundValue = false;

	json_object *boolObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &boolObject))
	{
		if (json_object_get_type(boolObject) == json_type_boolean)
		{
			b = json_object_get_boolean(boolObject);
			if (foundValue)
				*foundValue = true;
		}
	}

	return b;
}

/**
 * Parses the target set value for @a key in the top level of the JSON object.
 *
 * If no such value is found, returns an unrestricted target set.
 */
VuoCompilerTargetSet VuoCompilerModule::parseTargetSet(json_object *o, string key)
{
	VuoCompilerTargetSet t;
	json_object *operatingSystemsObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &operatingSystemsObject))
	{
		if (json_object_get_type(operatingSystemsObject) == json_type_object)
		{
			json_object *macosObject = NULL;
			if (json_object_object_get_ex(operatingSystemsObject, "macosx", &macosObject))
			{
				t.setMinMacVersion( parseMacVersion( parseString(macosObject, "min") ) );
				t.setMaxMacVersion( parseMacVersion( parseString(macosObject, "max") ) );
			}
		}
	}
	return t;
}

/**
 * Parses a Mac OS version from a string.
 *
 * If the string doesn't represent a Mac OS version, returns @c VuoCompilerTargetSet::MacVersion_Any.
 */
VuoCompilerTargetSet::MacVersion VuoCompilerModule::parseMacVersion(string version)
{
	if (version == "10.7")
		return VuoCompilerTargetSet::MacVersion_10_7;
	if (version == "10.8")
		return VuoCompilerTargetSet::MacVersion_10_8;
	if (version == "10.9")
		return VuoCompilerTargetSet::MacVersion_10_9;

	return VuoCompilerTargetSet::MacVersion_Any;
}

/**
 * Parses the array-of-strings value for @c key in the top level of the JSON object.
 *
 * If no such value is found, returns an empty vector.
 */
vector<string> VuoCompilerModule::parseArrayOfStrings(json_object *o, string key)
{
	vector<string> items;
	json_object *arrayObject = NULL;
	if (json_object_object_get_ex(o, key.c_str(), &arrayObject))
	{
		if (json_object_get_type(arrayObject) == json_type_array)
		{
			int itemCount = json_object_array_length(arrayObject);
			for (int i = 0; i < itemCount; ++i)
			{
				json_object *item = json_object_array_get_idx(arrayObject, i);
				if (json_object_get_type(item) == json_type_string)
					items.push_back( json_object_get_string(item) );
			}
		}
	}
	return items;
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
VuoCompilerTargetSet VuoCompilerModule::getCompatibleTargets(void)
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
 * Returns the (psuedo) base for this (psuedo) compiler detail class.
 */
VuoModule * VuoCompilerModule::getPseudoBase(void)
{
	return base;
}

/**
 * Returns a boolean indicating whether this module contains premium content.
 */
bool VuoCompilerModule::getPremium(void)
{
	return isPremium;
}

/**
 * Sets the boolean indicating whether this module contains premium content.
 * This attribute may be used, e.g., for purposes of automatic documentation generation.
 */
void VuoCompilerModule::setPremium(bool premium)
{
	this->isPremium = premium;
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
