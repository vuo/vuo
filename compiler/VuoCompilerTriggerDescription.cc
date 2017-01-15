/**
 * @file
 * VuoCompilerTriggerDescription implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#include "VuoCompilerNode.hh"
#include "VuoCompilerTriggerDescription.hh"
#include "VuoCompilerTriggerPort.hh"
#include "VuoNodeClass.hh"
#include "VuoType.hh"

/**
 * Returns a JSON representation of @a trigger that can later be parsed to create a VuoCompilerTriggerDescription.
 */
json_object * VuoCompilerTriggerDescription::getJson(VuoCompilerNode *triggerNode, VuoCompilerTriggerPort *trigger)
{
	string nodeIdentifier = triggerNode->getGraphvizIdentifier();
	string portName = trigger->getBase()->getClass()->getName();
	int portContextIndex = trigger->getIndexInPortContexts();
	VuoPortClass::EventThrottling eventThrottling = trigger->getBase()->getEventThrottling();
	string eventThrottlingStr = (eventThrottling == VuoPortClass::EventThrottling_Drop ? "drop" : "enqueue");
	VuoType *dataType = trigger->getDataVuoType();
	string dataTypeStr = (dataType ? dataType->getModuleKey() : "event");

	json_object *js = json_object_new_object();

	json_object_object_add(js, "nodeIdentifier", json_object_new_string(nodeIdentifier.c_str()));
	json_object_object_add(js, "portName", json_object_new_string(portName.c_str()));
	json_object_object_add(js, "portContextIndex", json_object_new_int(portContextIndex));
	json_object_object_add(js, "eventThrottling", json_object_new_string(eventThrottlingStr.c_str()));
	json_object_object_add(js, "dataType", json_object_new_string(dataTypeStr.c_str()));

	return js;
}

/**
 * Parses a JSON array of trigger descriptions.
 */
vector<VuoCompilerTriggerDescription *> VuoCompilerTriggerDescription::parseFromJson(json_object *js)
{
	vector<VuoCompilerTriggerDescription *> triggers;

	int itemCount = json_object_array_length(js);
	for (int i = 0; i < itemCount; ++i)
	{
		json_object *itemJs = json_object_array_get_idx(js, i);
		json_object *o = NULL;

		VuoCompilerTriggerDescription *trigger = new VuoCompilerTriggerDescription();

		if (json_object_object_get_ex(itemJs, "nodeIdentifier", &o))
			trigger->nodeIdentifier = json_object_get_string(o);
		if (json_object_object_get_ex(itemJs, "portName", &o))
			trigger->portName = json_object_get_string(o);
		if (json_object_object_get_ex(itemJs, "portContextIndex", &o))
			trigger->portContextIndex = json_object_get_int(o);
		if (json_object_object_get_ex(itemJs, "eventThrottling", &o))
		{
			string eventThrottlingStr = json_object_get_string(o);
			trigger->eventThrottling = (eventThrottlingStr == "drop" ? VuoPortClass::EventThrottling_Drop : VuoPortClass::EventThrottling_Enqueue);
		}
		if (json_object_object_get_ex(itemJs, "dataType", &o))
		{
			string dataTypeStr = json_object_get_string(o);
			trigger->dataType = (dataTypeStr == "event" ? NULL : new VuoType(dataTypeStr));
		}
		if (json_object_object_get_ex(itemJs, "subcompositionNodeClassName", &o))
			trigger->subcompositionNodeClassName = json_object_get_string(o);
		if (json_object_object_get_ex(itemJs, "subcompositionNodeIdentifier", &o))
			trigger->subcompositionNodeIdentifier = json_object_get_string(o);

		triggers.push_back(trigger);
	}

	return triggers;
}

/**
 * Returns a JSON representation of this trigger description that includes fields for @a subcompositionNode,
 * the subcomposition that contains the node on which the trigger appears.
 */
json_object * VuoCompilerTriggerDescription::getJsonWithinSubcomposition(VuoCompilerNode *subcompositionNode)
{
	string eventThrottlingStr = (getEventThrottling() == VuoPortClass::EventThrottling_Drop ? "drop" : "enqueue");
	string dataTypeStr = (dataType ? dataType->getModuleKey() : "event");
	string subcompositionNodeClassNameStr = (subcompositionNodeClassName.empty() ?
												 subcompositionNode->getBase()->getNodeClass()->getClassName() : subcompositionNodeClassName);
	string subcompositionNodeIdentifierStr = subcompositionNode->getGraphvizIdentifier() +
											 (subcompositionNodeIdentifier.empty() ?
												  "" : ("__" + subcompositionNodeIdentifier));

	json_object *js = json_object_new_object();

	json_object_object_add(js, "nodeIdentifier", json_object_new_string(nodeIdentifier.c_str()));
	json_object_object_add(js, "portName", json_object_new_string(portName.c_str()));
	json_object_object_add(js, "portContextIndex", json_object_new_int(portContextIndex));
	json_object_object_add(js, "eventThrottling", json_object_new_string(eventThrottlingStr.c_str()));
	json_object_object_add(js, "dataType", json_object_new_string(dataTypeStr.c_str()));
	json_object_object_add(js, "subcompositionNodeClassName", json_object_new_string(subcompositionNodeClassNameStr.c_str()));
	json_object_object_add(js, "subcompositionNodeIdentifier", json_object_new_string(subcompositionNodeIdentifierStr.c_str()));

	return js;
}

/**
 * Returns the identifier of the node on which the trigger port appears.
 */
string VuoCompilerTriggerDescription::getNodeIdentifier(void)
{
	return nodeIdentifier;
}

/**
 * Returns the name of the trigger port.
 */
string VuoCompilerTriggerDescription::getPortName(void)
{
	return portName;
}

/**
 * Returns the index of this trigger port within its node's port contexts.
 */
int VuoCompilerTriggerDescription::getPortContextIndex(void)
{
	return portContextIndex;
}

/**
 * Returns the event throttling setting of the trigger port.
 */
VuoPortClass::EventThrottling VuoCompilerTriggerDescription::getEventThrottling(void)
{
	return eventThrottling;
}

/**
 * Returns the data type of the trigger port.
 */
VuoType * VuoCompilerTriggerDescription::getDataType(void)
{
	return dataType;
}

/**
 * Sets the data type of the trigger port.
 */
void VuoCompilerTriggerDescription::setDataType(VuoType *dataType)
{
	this->dataType = dataType;
}

/**
 * Returns the node class name of the subcomposition that contains the node on which this trigger appears.
 */
string VuoCompilerTriggerDescription::getSubcompositionNodeClassName(void)
{
	return subcompositionNodeClassName;
}

/**
 * Returns the node identifier of the subcomposition that contains the node on which this trigger appears.
 */
string VuoCompilerTriggerDescription::getSubcompositionNodeIdentifier(void)
{
	return subcompositionNodeIdentifier;
}
