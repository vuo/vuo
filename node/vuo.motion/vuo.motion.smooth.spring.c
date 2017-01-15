/**
 * @file
 * vuo.motion.spring node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <math.h>

VuoModuleMetadata({
					  "title" : "Smooth with Spring",
					  "keywords" : [ "bounce", "elastic", "oscillate", "rebound", "ricochet", "wobble", "harmonic", "sine", "sinusoidial" ],
					  "version" : "2.1.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "SpringBack.vuo" ]
					  }
				 });

struct nodeInstanceData
{
	bool first;
	bool moving;
	bool updateNeededAfterSetPosition;
	VuoGenericType1 startPosition;
	VuoGenericType1 currentPosition;
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
	VuoInputData(VuoReal, {"default":0.0}) time,
	VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
	VuoInputData(VuoGenericType1, {"defaults":{
									   "VuoReal":1.,
									   "VuoPoint2d":{"x":1.,"y":1.},
									   "VuoPoint3d":{"x":1.,"y":1.,"z":0.}
								   }}) setPosition,
	VuoInputEvent({"eventBlocking":"wall","data":"setPosition","hasPortAction":true}) setPositionEvent,
	VuoInputData(VuoGenericType1, {"defaults":{
									   "VuoReal":0.,
									   "VuoPoint2d":{"x":0.,"y":0.},
									   "VuoPoint3d":{"x":0.,"y":0.,"z":0.}
								   }}) setTarget,
	VuoInputEvent({"eventBlocking":"wall","data":"setTarget","hasPortAction":true}) setTargetEvent,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.000001, "suggestedStep":0.1}) period,
	VuoInputEvent({"eventBlocking":"wall","data":"period"}) periodEvent,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) damping,
	VuoInputEvent({"eventBlocking":"wall","data":"damping"}) dampingEvent,

	VuoOutputData(VuoGenericType1) position,
	VuoOutputEvent({"data":"position"}) positionEvent,
	VuoOutputEvent() reachedTarget,

	VuoInstanceData(struct nodeInstanceData *) ctx
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
		double positivePeriod = MAX(period,0.000001);
		double clampedDamping = MAX(MIN(damping,0.999999),0.000001);
		VuoReal timeSinceStart = time - (*ctx)->timeMotionStarted;
		(*ctx)->currentPosition = VuoGenericType1_spring(timeSinceStart, (*ctx)->startPosition, setTarget, period, clampedDamping);

		if (timeSinceStart > 2.*period/clampedDamping)
		{
			(*ctx)->moving = false;
			*reachedTarget = true;
		}
	}

	*position = (*ctx)->currentPosition;
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) ctx)
{
}
