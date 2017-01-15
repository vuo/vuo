/**
 * @file
 * VuoCompilerPublishedInputNodeClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDINPUTNODECLASS_H
#define VUOCOMPILERPUBLISHEDINPUTNODECLASS_H

#include "VuoCompilerSpecializedNodeClass.hh"

class VuoPublishedPort;

/**
 * A node class used when generating code for a composition to represent published input ports.
 * For each published input port, this node class has an input port with the same name and data type.
 * In addition, this node class has an event-only trigger output port.
 */
class VuoCompilerPublishedInputNodeClass : public VuoCompilerSpecializedNodeClass
{
public:
	static VuoNodeClass * newNodeClass(vector<VuoPublishedPort *> publishedInputPorts);
	VuoCompilerNode * createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler);
	VuoType * getOriginalPortType(VuoPortClass *portClass);
	string getOriginalGenericNodeClassName(void);
	string getOriginalGenericNodeClassDescription(void);
	VuoNodeSet * getOriginalGenericNodeSet(void);
	string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);

private:
	VuoCompilerPublishedInputNodeClass(string nodeClassName, Module *module);
	VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *compilerNodeClass);
	VuoCompilerPublishedInputNodeClass(VuoNodeClass *baseNodeClass);
};

#endif
