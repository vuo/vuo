/**
 * @file
 * VuoCompilerTriggerDescription interface.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#pragma once

class VuoCompilerGraph;
class VuoCompilerNode;
class VuoCompilerTriggerPort;
class VuoType;

#include "VuoPortClass.hh"

/**
 * Information about a trigger port inside a subcomposition that the subcomposition advertises to its containing composition.
 */
class VuoCompilerTriggerDescription
{
public:
	static json_object * getJson(VuoCompilerNode *triggerNode, VuoCompilerTriggerPort *trigger, VuoCompilerGraph *graph);
	static vector<VuoCompilerTriggerDescription *> parseFromJson(json_object *js);
	json_object * getJsonWithinSubcomposition(VuoCompilerNode *subcompositionNode);
	size_t getNodeIndex(void);
	string getNodeIdentifier(void);
	string getPortName(void);
	int getPortContextIndex(void);
	VuoPortClass::EventThrottling getEventThrottling(void);
	VuoType * getDataType(void);
	void setDataType(VuoType *dataType);
	void getWorkerThreadsNeeded(int &minThreadsNeeded, int &maxThreadsNeeded);
	int getChainCount(void);
	string getSubcompositionNodeClassName(void);
	string getSubcompositionNodeIdentifier(void);

private:
	VuoCompilerTriggerDescription(void);

	size_t nodeIndex;
	string nodeIdentifier;
	string portName;
	int portContextIndex;
	VuoPortClass::EventThrottling eventThrottling;
	VuoType *dataType;
	int minWorkerThreadsNeeded;
	int maxWorkerThreadsNeeded;
	int chainCount;
	string subcompositionNodeClassName;
	string subcompositionNodeIdentifier;
};
