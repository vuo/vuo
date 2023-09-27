/**
 * @file
 * VuoCompilerCompoundType interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerType.hh"
class VuoCompiler;

/**
 * A specialization of a type whose original definition refers to one or more generic types.
 * For example, the compound type could be a list, a dictionary, or a pair.
 */
class VuoCompilerCompoundType : public VuoCompilerType
{
public:
	static VuoCompilerCompoundType * newType(const string &typeName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoCompilerCompoundType * newType(const string &typeName, Module *module);
	static string parseGenericCompoundTypeName(const string &specializedCompoundTypeName, size_t genericTypeCount);
	static map<string, string> parseSpecializedTypes(const string &specializedCompoundTypeName, const string &genericTypeName);
	static string buildSpecializedCompoundTypeName(const string &genericCompoundTypeName, const vector<string> &specializedTypeNames);
	static string buildUnspecializedCompoundTypeName(const string &genericCompoundTypeName, size_t genericTypeCount);
	static json_object * buildSpecializedModuleDetails(VuoCompilerType *genericCompoundType, const map<string, string> &specializedForGenericTypeName, const map<string, VuoCompilerType *> &specializedTypes);
	static string buildDefaultTitle(const string &specializedCompoundTypeName, std::function<VuoCompilerType *(const string &)> getVuoType);

protected:
	VuoCompilerCompoundType(const string &typeName, Module *module);

private:
	static VuoCompilerType * parseGenericCompoundType(const string &specializedCompoundTypeName, std::function<VuoCompilerType *(const string &)> getVuoType);
	static string buildDefaultTitle(VuoCompilerType *genericCompoundType, const map<string, string> &specializedForGenericTypeName, const map<string, VuoCompilerType *> &specializedTypes);
	static string getSourceCode(VuoCompilerType *genericCompoundType, string &sourcePath);
};
