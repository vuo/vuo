/**
 * @file
 * VuoCompilerTriggerDescription interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This interface description may be modified and distributed under the terms of the GNU Lesser General Public License (LGPL) version 2 or later.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOCOMPILERTRIGGERDESCRIPTION_H
#define VUOCOMPILERTRIGGERDESCRIPTION_H

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
	static json_object * getJson(VuoCompilerNode *triggerNode, VuoCompilerTriggerPort *trigger);
	static vector<VuoCompilerTriggerDescription *> parseFromJson(json_object *js);
	json_object * getJsonWithinSubcomposition(VuoCompilerNode *subcompositionNode);
	string getNodeIdentifier(void);
	string getPortName(void);
	int getPortContextIndex(void);
	VuoPortClass::EventThrottling getEventThrottling(void);
	VuoType * getDataType(void);
	void setDataType(VuoType *dataType);
	string getSubcompositionNodeClassName(void);
	string getSubcompositionNodeIdentifier(void);

private:
	string nodeIdentifier;
	string portName;
	int portContextIndex;
	VuoPortClass::EventThrottling eventThrottling;
	VuoType *dataType;
	string subcompositionNodeClassName;
	string subcompositionNodeIdentifier;
};

#endif
