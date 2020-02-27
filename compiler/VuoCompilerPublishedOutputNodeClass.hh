/**
 * @file
 * VuoCompilerPublishedOutputNodeClass interface.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#include "VuoCompilerPublishedNodeClass.hh"

/**
 * A node class used when generating code for a composition to handle data storage of and
 * event flow through published output ports.
 */
class VuoCompilerPublishedOutputNodeClass : public VuoCompilerPublishedNodeClass
{
public:
	static VuoNodeClass * newNodeClass(string nodeClassName, VuoCompiler *compiler, dispatch_queue_t llvmQueue);
	static VuoNodeClass * newNodeClass(vector<VuoPublishedPort *> publishedOutputPorts, dispatch_queue_t llvmQueue = nullptr);
	size_t getInputPortIndexForPublishedOutputPort(size_t publishedOutputPortIndex);
	size_t getGatherInputPortIndex(void);
	static string buildNodeClassName(const vector<VuoPublishedPort *> &publishedOutputPorts);

private:
	static VuoCompilerPublishedOutputNodeClass * getSingleton(void);
	VuoCompilerPublishedOutputNodeClass(string nodeClassName, Module *module);
	VuoCompilerPublishedOutputNodeClass(VuoCompilerPublishedOutputNodeClass *compilerNodeClass);
	VuoCompilerPublishedOutputNodeClass(VuoNodeClass *baseNodeClass);
	VuoNodeClass * newNodeClassWithImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types);
	VuoNodeClass * newNodeClassWithoutImplementation(const string &nodeClassName, const vector<string> &portNames, const vector<VuoType *> &types);

	string getNodeClassNamePrefix(void);
	set<string> getReservedPortNames(void);

	static VuoCompilerPublishedOutputNodeClass *singleton;
};
