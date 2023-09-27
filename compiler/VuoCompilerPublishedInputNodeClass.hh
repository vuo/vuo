/**
 * @file
 * VuoCompilerPublishedInputNodeClass interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerPublishedNodeClass.hh"

/**
 * A node class used when generating code for a composition to handle data storage of and
 * event flow through published input ports.
 */
class VuoCompilerPublishedInputNodeClass : public VuoCompilerPublishedNodeClass
{
public:
	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoNodeClass * newNodeClass(vector<VuoPublishedPort *> publishedInputPorts, dispatch_queue_t llvmQueue = nullptr);
	static VuoNodeClass * newNodeClass(string nodeClassName, Module *module);
	size_t getInputPortIndexForPublishedInputPort(size_t publishedInputPortIndex);
	size_t getOutputPortIndexForPublishedInputPort(size_t publishedInputPortIndex);
	static string buildNodeClassName(const vector<VuoPublishedPort *> &publishedInputPorts);

private:
	static VuoCompilerPublishedInputNodeClass * getSingleton(void);
	VuoCompilerPublishedInputNodeClass(string nodeClassName, Module *module);
	VuoCompilerPublishedInputNodeClass(VuoCompilerPublishedInputNodeClass *compilerNodeClass);
	VuoCompilerPublishedInputNodeClass(VuoNodeClass *baseNodeClass);
	VuoNodeClass * newNodeClassWithImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types);
	VuoNodeClass * newNodeClassWithoutImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types);

	string getNodeClassNamePrefix(void);
	set<string> getReservedPortNames(void);

	static VuoCompilerPublishedInputNodeClass *singleton;
};
