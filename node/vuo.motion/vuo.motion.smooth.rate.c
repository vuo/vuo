/**
 * @file
 * vuo.motion.smooth.rate node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Smooth with Rate",
					  "keywords" : [ "ease", "easing", "even", "calm", "steady", "continuous", "transition", "time", "speed", "velocity" ],
					  "version" : "1.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "CompareSmoothedMotion.vuo", "CompareSmoothedData.vuo", "RotateInSequence.vuo" ]
					  }
				 });


struct nodeInstanceData
{
	bool first;
	bool moving;
	bool updateNeededAfterSetPosition;
	VuoGenericType1 startPosition;
	VuoGenericType1 currentPosition;
	VuoReal distanceFromStartToTarget;
	VuoReal timeMotionStarted;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);
	ctx->first = true;
	return ctx;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":0.,
										   "VuoPoint2d":{"x":0.,"y":0.},
										   "VuoPoint3d":{"x":0.,"y":0.,"z":0.}
									   }}) setPosition,
		VuoInputEvent({"eventBlocking":"wall","data":"setPosition","hasPortAction":true}) setPositionEvent,
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":1.,
										   "VuoPoint2d":{"x":1.,"y":1.},
										   "VuoPoint3d":{"x":1.,"y":1.,"z":1.}
									   }}) setTarget,
		VuoInputEvent({"eventBlocking":"wall","data":"setTarget","hasPortAction":true}) setTargetEvent,
		VuoInputData(VuoReal, {"default":1., "suggestedMin":0., "suggestedStep":0.1}) rate,
		VuoInputEvent({"eventBlocking":"wall","data":"rate"}) rateEvent,
		VuoInputData(VuoCurve, {"default":"linear"}) curve,
		VuoInputEvent({"eventBlocking":"wall","data":"curve"}) curveEvent,
		VuoInputData(VuoCurveEasing, {"default":"in"}) easing,
		VuoInputEvent({"eventBlocking":"wall","data":"easing"}) easingEvent,

		VuoOutputData(VuoGenericType1) position,
		VuoOutputEvent({"data":"position"}) positionEvent,
		VuoOutputEvent() reachedTarget
)
{
	if ((*ctx)->first || setPositionEvent)
	{
		(*ctx)->currentPosition = setPosition;
		(*ctx)->moving = false;
		if(setPositionEvent)
			(*ctx)->updateNeededAfterSetPosition = true;
		(*ctx)->first = false;
	}

	if (setTargetEvent)
	{
		(*ctx)->startPosition = (*ctx)->currentPosition;
		(*ctx)->distanceFromStartToTarget = VuoGenericType1_distance((*ctx)->startPosition, setTarget);
		(*ctx)->timeMotionStarted = time;
		(*ctx)->moving = true;
	}

	if (timeEvent && ((*ctx)->moving || (*ctx)->updateNeededAfterSetPosition))
	{
		*positionEvent = true;
		(*ctx)->updateNeededAfterSetPosition = false;
	}

	if ((*ctx)->moving)
	{
		VuoReal timeSinceStart = time - (*ctx)->timeMotionStarted;
		VuoReal duration = (*ctx)->distanceFromStartToTarget / rate;
		(*ctx)->currentPosition = VuoGenericType1_curve(timeSinceStart, (*ctx)->startPosition, setTarget, duration, curve, easing, VuoLoopType_None);

		if (timeSinceStart > duration)
		{
			(*ctx)->moving = false;
			*reachedTarget = true;
		}
	}

	*position = (*ctx)->currentPosition;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
}
