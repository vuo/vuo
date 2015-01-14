/**
 * @file
 * vuo.leap node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLeap.h"

VuoModuleMetadata({
					  "title" : "Receive Leap Frame",
					  "description" :
						  "<p>Fires an event each time the Leap Motion device provides new hand- and finger-tracking data.</p> \
						  <p>Data from the Leap Motion is grouped into frames. Each frame contains information about hands \
						  and pointables (fingers and tools) detected at one moment in time.</p> \
						  <p>To see the information within a frame, use a `Get Frame Values` node.</p> \
						  ",
					  "keywords" : [ "gesture", "controller", "motion", "hand", "palm", "pointable", "finger", "tool" ],
					  "version" : "1.0.0",
					  "dependencies" : [
						  "VuoLeap"
					  ],
					  "node": {
						  "isInterface" : false
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
		VuoOutputTrigger(receivedFrame, VuoLeapFrame)
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
