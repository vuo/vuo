/**
 * @file
 * VuoCompilerMakeListNodeClass interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERMAKELISTNODECLASS_H
#define VUOCOMPILERMAKELISTNODECLASS_H

#include "VuoCompilerNodeClass.hh"

/**
 * A "Make List" node class.
 */
class VuoCompilerMakeListNodeClass : public VuoCompilerNodeClass
{
private:
	unsigned long itemCount;
	VuoCompilerType *listType;

	VuoCompilerMakeListNodeClass(string nodeClassName, Module *module);
	VuoCompilerMakeListNodeClass(VuoCompilerMakeListNodeClass *compilerNodeClass);
	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler);

	static const string makeListNodeClassNamePrefix;
	static const string listTypeNamePrefix;

public:
	unsigned long getItemCount(void);
	VuoCompilerType * getListType(void);

	static VuoCompilerMakeListNodeClass * getNodeClass(string nodeClassName, VuoCompiler *compiler);
	static bool isMakeListNodeClassName(string nodeClassName);
	static bool isListType(VuoCompilerType *type);
	static string getNodeClassName(unsigned long itemCount, VuoCompilerType *listType);
};

#endif
