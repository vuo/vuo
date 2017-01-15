/**
 * @file
 * VuoCompilerMakeListNodeClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERMAKELISTNODECLASS_H
#define VUOCOMPILERMAKELISTNODECLASS_H

#include "VuoCompilerSpecializedNodeClass.hh"

class VuoCompilerType;

/**
 * A "Make List" node class.
 */
class VuoCompilerMakeListNodeClass : public VuoCompilerSpecializedNodeClass
{
private:
	unsigned long itemCount;
	VuoCompilerType *listType;

	VuoCompilerMakeListNodeClass(string nodeClassName, Module *module);
	VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass);
	VuoCompilerMakeListNodeClass(VuoNodeClass *baseNodeClass);
	void initialize(void);

	static bool parseNodeClassName(string nodeClassName, unsigned long &itemCount, string &itemTypeName);

	static const string makeListNodeClassNamePrefix;
	static const string makeListNodeClassDescription;

public:
	unsigned long getItemCount(void);
	VuoCompilerType * getListType(void);
	VuoType * getOriginalPortType(VuoPortClass *portClass);
	string getOriginalGenericNodeClassName(void);
	string getOriginalGenericNodeClassDescription(void);
	VuoNodeSet * getOriginalGenericNodeSet(void);
	string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);

	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static bool isMakeListNodeClassName(string nodeClassName);
	static string getNodeClassName(unsigned long itemCount, VuoCompilerType *listType);
	static string buildNodeClassName(unsigned long itemCount, string itemTypeName);
};

#endif
