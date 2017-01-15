/**
 * @file
 * VuoCompilerPublishedOutputNodeClass interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERPUBLISHEDOUTPUTNODECLASS_H
#define VUOCOMPILERPUBLISHEDOUTPUTNODECLASS_H

#include "VuoCompilerSpecializedNodeClass.hh"

class VuoPublishedPort;

/**
 * A node class used when generating code for a composition to represent published output ports.
 * For each published output port, this node class has an input port with the same name and data type.
 */
class VuoCompilerPublishedOutputNodeClass : public VuoCompilerSpecializedNodeClass
{
public:
	static VuoNodeClass * newNodeClass(vector<VuoPublishedPort *> publishedOutputPorts);
	VuoCompilerNode * createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler);
	VuoType * getOriginalPortType(VuoPortClass *portClass);
	string getOriginalGenericNodeClassName(void);
	string getOriginalGenericNodeClassDescription(void);
	VuoNodeSet * getOriginalGenericNodeSet(void);
	string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);

private:
	VuoCompilerPublishedOutputNodeClass(string nodeClassName, Module *module);
	VuoCompilerPublishedOutputNodeClass(VuoCompilerPublishedOutputNodeClass *compilerNodeClass);
	VuoCompilerPublishedOutputNodeClass(VuoNodeClass *baseNodeClass);
};

#endif
