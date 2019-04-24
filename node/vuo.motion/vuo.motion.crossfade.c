/**
 * @file
 * vuo.motion.crossfade node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Crossfade",
					  "keywords" : [ "transition", "dissolve" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "CrossfadeRectangleSizes.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	bool status;	// false = target is 1; true = target is 2
	VuoGenericType1 startPosition1;
	VuoGenericType1 startPosition2;
	VuoGenericType1 currentPosition1;
	VuoGenericType1 currentPosition2;
	VuoReal timeMotionStarted;
	bool moving;
};

struct nodeInstanceData *nodeInstanceInit
(
	VuoInputData(VuoGenericType1) offValue,
	VuoInputData(VuoGenericType1) onValue
)
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);

	// Output the starting positions the first time we receive a `time` event.
	ctx->status = false;
	ctx->startPosition1 = ctx->currentPosition1 =  onValue;
	ctx->startPosition2 = ctx->currentPosition2 = offValue;
	ctx->timeMotionStarted = -INFINITY;
	ctx->moving = true;

	return ctx;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) ctx,
	VuoInputData(VuoReal, {"default":0.0}) time,
	VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
	VuoInputEvent({"eventBlocking":"wall"}) toggle,
	VuoInputEvent({"eventBlocking":"wall","name":"Crossfade to 1"}) crossfadeTo1,
	VuoInputEvent({"eventBlocking":"wall","name":"Crossfade to 2"}) crossfadeTo2,
	VuoInputData(VuoGenericType1, {"defaults":{
									   "VuoReal":0.,
									   "VuoPoint2d":{"x":0.,"y":0.},
									   "VuoPoint3d":{"x":0.,"y":0.,"z":0.},
									   "VuoPoint4d":{"x":0.,"y":0.,"z":0.,"w":0.}
								   }}) offValue,
	VuoInputEvent({"eventBlocking":"wall","data":"offValue"}) offValueEvent,
	VuoInputData(VuoGenericType1, {"defaults":{
									   "VuoReal":1.,
									   "VuoPoint2d":{"x":1.,"y":1.},
									   "VuoPoint3d":{"x":1.,"y":1.,"z":1.},
									   "VuoPoint4d":{"x":1.,"y":1.,"z":1.,"w":1.}
								   }}) onValue,
	VuoInputEvent({"eventBlocking":"wall","data":"onValue"}) onValueEvent,
	VuoInputData(VuoReal, {"default":1., "suggestedMin":0., "suggestedStep":0.1}) duration,
	VuoInputEvent({"eventBlocking":"wall","data":"duration"}) durationEvent,
	VuoInputData(VuoCurve, {"default":"quadratic"}) curve,
	VuoInputEvent({"eventBlocking":"wall","data":"curve"}) curveEvent,
	VuoInputData(VuoCurveEasing, {"default":"out"}) easing,
	VuoInputEvent({"eventBlocking":"wall","data":"easing"}) easingEvent,

	VuoOutputData(VuoGenericType1) channel1Value,
	VuoOutputEvent({"data":"channel1Value"}) channel1Event,
	VuoOutputData(VuoGenericType1) channel2Value,
	VuoOutputEvent({"data":"channel2Value"}) channel2Event,
	VuoOutputEvent() reachedTarget
)
{
	if (toggle || crossfadeTo1 || crossfadeTo2)
	{
		bool statusChanged = false;
		if (toggle)
		{
			(*ctx)->status = !(*ctx)->status;
			statusChanged = true;
		}
		else if (crossfadeTo1 && (*ctx)->status != false)
		{
			(*ctx)->status = false;
			statusChanged = true;
		}
		else if (crossfadeTo2 && (*ctx)->status != true)
		{
			(*ctx)->status = true;
			statusChanged = true;
		}

		if (statusChanged)
		{
			(*ctx)->timeMotionStarted = time;
			(*ctx)->startPosition1 = (*ctx)->currentPosition1;
			(*ctx)->startPosition2 = (*ctx)->currentPosition2;
			(*ctx)->moving = true;
		}
	}

	if (timeEvent && (*ctx)->moving)
	{
		VuoReal elapsed = time - (*ctx)->timeMotionStarted;
		(*ctx)->currentPosition1 = *channel1Value = VuoGenericType1_curve(elapsed, (*ctx)->startPosition1, (*ctx)->status ? offValue :  onValue, duration, curve, easing, VuoLoopType_None);
		(*ctx)->currentPosition2 = *channel2Value = VuoGenericType1_curve(elapsed, (*ctx)->startPosition2, (*ctx)->status ?  onValue : offValue, duration, curve, easing, VuoLoopType_None);
		*channel1Event = true;
		*channel2Event = true;

		if (elapsed >= duration)
		{
			*reachedTarget = true;
			(*ctx)->moving = false;
		}
	}
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) ctx
)
{
}
