/**
 * @file
 * vuo.time.changeSpeed node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Change Speed",
					  "keywords" : [ "retime", "faster", "fast forward", "slower", "slow motion", "reverse", "backwards", "rewind",
							 "multiply", "adjust", "accelerate", "decelerate", "speed up", "slow down" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "ChangeSpeedWithMouse.vuo" ]
					  },
				  });

struct nodeInstanceData
{
	VuoReal accumulatedTime;
	VuoReal priorTime;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *) calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}
void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoReal, {"default":0.0}) time,
	VuoInputEvent({"eventBlocking":"none", "data":"time", "hasPortAction":false}) timeEvent,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":-4, "suggestedMax":4}) speed,
	VuoInputEvent({"eventBlocking":"wall", "data":"speed"}) speedEvent,
	VuoInputEvent({"eventBlocking":"wall"}) reset,
	VuoOutputData(VuoReal) changedTime,
	VuoOutputEvent({"data":"changedTime"}) changedTimeEvent
)
{
	if (reset)
		(*context)->accumulatedTime = 0;

	if (timeEvent)
	{
		(*context)->accumulatedTime += (time - (*context)->priorTime) * speed;

		*changedTime = (*context)->accumulatedTime;
		*changedTimeEvent = true;

		(*context)->priorTime = time;
	}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
}
