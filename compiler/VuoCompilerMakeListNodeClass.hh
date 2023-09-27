/**
 * @file
 * VuoCompilerMakeListNodeClass interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerSpecializedNodeClass.hh"

class VuoCompilerType;

/**
 * A "Make List" node class.
 */
class VuoCompilerMakeListNodeClass : public VuoCompilerSpecializedNodeClass
{
private:
	unsigned long itemCount;
	VuoCompilerType *listType;  ///< The node class with name `listTypeName`, or null if it hasn't yet been filled in by `updateListType()`.
	string listTypeName;  ///< The name of the generic node class that this node class specializes.

	VuoCompilerMakeListNodeClass(string nodeClassName, Module *module);
	VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass);
	VuoCompilerMakeListNodeClass(VuoNodeClass *baseNodeClass);

	static const string makeListNodeClassNamePrefix;
	static const string makeListNodeClassDescription;

public:
	unsigned long getItemCount(void);
	VuoCompilerType * getListType(void);
	bool updateListType(std::function<VuoCompilerType *(const string &)> lookUpType);
	VuoType * getOriginalPortType(VuoPortClass *portClass);
	string getOriginalGenericNodeClassName(void);
	string getOriginalGenericNodeClassDescription(void);
	VuoNodeSet * getOriginalGenericNodeSet(void);
	string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);

	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoNodeClass *newNodeClass(string nodeClassName, Module *module);
	static bool isMakeListNodeClassName(string nodeClassName);
	static string getNodeClassName(unsigned long itemCount, VuoCompilerType *listType);
	static bool parseNodeClassName(string nodeClassName, unsigned long &itemCount, string &itemTypeName);
	static string buildNodeClassName(unsigned long itemCount, string itemTypeName);
};
