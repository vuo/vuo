/**
 * @file
 * VuoCompilerPublishedNodeClass interface.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerSpecializedNodeClass.hh"
#include "VuoHeap.h"

class VuoPublishedPort;

/**
 * Virtual base class for node classes used to handle published ports when generating code.
 */
class VuoCompilerPublishedNodeClass : public VuoCompilerSpecializedNodeClass
{
protected:
	VuoCompilerPublishedNodeClass(string nodeClassName, Module *module);
	VuoCompilerPublishedNodeClass(VuoCompilerPublishedNodeClass *compilerNodeClass);
	VuoCompilerPublishedNodeClass(VuoNodeClass *baseNodeClass);
	static VuoNodeClass * newNodeClass(const string &nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton);
	static VuoNodeClass * newNodeClass(const vector<VuoPublishedPort *> &publishedPorts, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton);

	/// Returns a node class with generated code in an LLVM module.
	virtual VuoNodeClass * newNodeClassWithImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types) = 0;

	/// Returns a node class without generated code.
	virtual VuoNodeClass * newNodeClassWithoutImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types) = 0;

	/// Returns the prefix that appears in any generic or specialized node class name for the node class.
	virtual string getNodeClassNamePrefix(void) = 0;

	/// Returns the names of input port classes that are added automatically to all published input/output node classes.
	virtual set<string> getReservedPortNames(void) = 0;

	bool parseNodeClassName(string nodeClassName, vector<string> &portNames, vector<string> &typeNames) VuoWarnUnusedResult;
	string buildNodeClassName(const vector<string> &portNames, const vector<VuoType *> &types);
	string buildNodeClassNameFromPorts(const vector<VuoPublishedPort *> &publishedPorts);
	vector<string> formUniquePortNames(const vector<VuoPublishedPort *> &publishedPorts);

	VuoCompilerNode * createReplacementBackingNode(VuoNode *nodeToBack, string backingNodeClassName, VuoCompiler *compiler);
	VuoType * getOriginalPortType(VuoPortClass *portClass);
	string getOriginalGenericNodeClassName(void);
	string getOriginalGenericNodeClassDescription(void);
	VuoNodeSet * getOriginalGenericNodeSet(void);
	string createUnspecializedNodeClassName(set<VuoPortClass *> portClassesToUnspecialize);
	string createSpecializedNodeClassNameWithReplacement(string genericTypeName, string specializedTypeName);

private:
	static VuoNodeClass * newNodeClass(const vector<string> &portNames, const vector<VuoType *> &types, dispatch_queue_t llvmQueue, VuoCompilerPublishedNodeClass *singleton);
};
