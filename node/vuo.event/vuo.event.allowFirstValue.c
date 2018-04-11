/**
 * @file
 * vuo.data.allowFirst node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Allow First Value",
					  "keywords" : [ "filter", "hold", "block", "prevent", "pass", "once", "single", "start", "initialize", "data" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "TakeSnapshotFromLiveVideo.vuo" ]
					  }
				  });

VuoBoolean * nodeInstanceInit()
{
	VuoBoolean *receivedEvent = (VuoBoolean *)malloc(sizeof(VuoBoolean));
	VuoRegister(receivedEvent, free);
	*receivedEvent = false;
	return receivedEvent;
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoBoolean *) receivedEvent,
		VuoInputData(VuoGenericType1) value,
		VuoInputEvent({"eventBlocking":"door", "data":"value", "hasPortAction":true}) event,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoGenericType1) firstValue,
		VuoOutputEvent({"data":"firstValue"}) firstValueEvent
)
{
	if(reset)
	{
		**receivedEvent = false;
	}

	if( event && !**receivedEvent )
	{
		*firstValue = value;
		**receivedEvent = true;
		*firstValueEvent = true;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(VuoBoolean *) receivedEvent
)
{
}
