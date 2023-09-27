/**
 * @file
 * vuo.motion.smooth.inertia node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSmooth_VuoGenericType1.h"

VuoModuleMetadata({
					  "title" : "Smooth with Inertia",
					  "keywords" : [ "ease", "easing", "even", "calm", "steady", "continuous", "transition", "time", "speed", "velocity", "gravity", "attraction", "momentum" ],
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
	VuoSmoothInertia smooth;
	bool moving;
};

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":0.,
										   "VuoPoint2d":{"x":0.,"y":0.},
										   "VuoPoint3d":{"x":0.,"y":0.,"z":0.}
									   }}) setPosition
)
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);
	ctx->smooth = VuoSmoothInertia_make(setPosition);
	VuoRetain(ctx->smooth);
	ctx->moving = false;
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
		VuoInputData(VuoReal, {"default":1., "suggestedMin":0., "suggestedStep":0.1}) duration,
		VuoInputEvent({"eventBlocking":"wall","data":"duration"}) durationEvent,

		VuoOutputData(VuoGenericType1) position,
		VuoOutputEvent({"data":"position"}) positionEvent,
		VuoOutputEvent() reachedTarget
)
{
	if (setPositionEvent)
	{
		VuoSmoothInertia_setPosition((*ctx)->smooth, setPosition);
		*position = setPosition;
		(*ctx)->moving = true;
	}

	if (setTargetEvent)
	{
		VuoSmoothInertia_setTarget((*ctx)->smooth, time, setTarget);
		(*ctx)->moving = true;
	}

	if (timeEvent && (*ctx)->moving)
	{
		VuoSmoothInertia_setDuration((*ctx)->smooth, duration);
		*positionEvent = VuoSmoothInertia_step((*ctx)->smooth, time, position);
		if (!*positionEvent)
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
	VuoRelease((*ctx)->smooth);
}
