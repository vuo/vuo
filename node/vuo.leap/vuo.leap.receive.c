/**
 * @file
 * vuo.leap.receive node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeap.h"

VuoModuleMetadata({
					  "title" : "Receive Leap Frame",
					  "keywords" : [ "gesture", "controller", "motion", "hand", "palm", "pointable", "finger", "tool" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoLeap"
					  ],
					  "node": {
						  "isInterface" : true,
						  "exampleCompositions" : [ "CountLeapObjects.vuo", "TwirlImageWithLeap.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	VuoLeap *leap;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(receivedFrame, VuoLeapFrame, VuoPortEventThrottling_Drop)
)
{
	(*context)->leap = VuoLeap_startListening(receivedFrame);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoLeap_stopListening((*context)->leap);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
