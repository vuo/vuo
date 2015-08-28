/**
 * @file
 * vuo.motion.smooth.inertia node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Smooth with Inertia",
					  "keywords" : [ "ease", "easing", "even", "calm", "steady", "continuous", "transition", "time", "speed", "velocity", "gravity", "attraction", "momentum" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "CompareSmoothedMotion.vuo", "CompareSmoothedData.vuo" ]
					  }
				 });


struct nodeInstanceData
{
	bool first;
	bool moving;
	bool updateNeededAfterSetPosition;
	VuoGenericType1 positionLastFrame;
	VuoGenericType1 velocity;
	VuoGenericType1 positionWhenTargetChanged;
	VuoGenericType1 velocityWhenTargetChanged;
	VuoReal timeLastFrame;
	VuoReal timeWhenTargetChanged;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *ctx = (struct nodeInstanceData *) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(ctx, free);
	ctx->first = true;
	ctx->timeLastFrame = ctx->timeWhenTargetChanged = 0;
	return ctx;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) ctx,
		VuoInputData(VuoReal, {"default":0.0}) time,
		VuoInputEvent(VuoPortEventBlocking_Door, time) timeEvent,
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":0.,
										   "VuoPoint2d":{"x":0.,"y":0.},
										   "VuoPoint3d":{"x":0.,"y":0.,"z":0.}
									   }}) setPosition,
		VuoInputEvent(VuoPortEventBlocking_Wall, setPosition, {"hasPortAction":true}) setPositionEvent,
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":1.,
										   "VuoPoint2d":{"x":1.,"y":1.},
										   "VuoPoint3d":{"x":1.,"y":1.,"z":1.}
									   }}) setTarget,
		VuoInputEvent(VuoPortEventBlocking_Wall, setTarget, {"hasPortAction":true}) setTargetEvent,
		VuoInputData(VuoReal, {"default":1., "suggestedMin":0., "suggestedStep":0.1}) duration,
		VuoInputEvent(VuoPortEventBlocking_Wall, duration) durationEvent,
		VuoOutputData(VuoGenericType1) position,
		VuoOutputEvent(position) positionEvent
)
{
	if ((*ctx)->first || setPositionEvent)
	{
		(*ctx)->positionLastFrame = *position = setPosition;
		(*ctx)->moving = false;
		(*ctx)->velocity = VuoGenericType1_valueFromString("");
		if( setPositionEvent )	
			(*ctx)->updateNeededAfterSetPosition = true;
		(*ctx)->first = false;
	}

	if (setTargetEvent)
	{
		(*ctx)->positionWhenTargetChanged = *position;
		(*ctx)->velocityWhenTargetChanged = (*ctx)->velocity;
		(*ctx)->timeWhenTargetChanged = time;
		(*ctx)->moving = true;
	}

	if (timeEvent)
	{
		if ((*ctx)->moving || (*ctx)->updateNeededAfterSetPosition)
		{
			*positionEvent = true;
			(*ctx)->updateNeededAfterSetPosition = false;
		}

		if ((*ctx)->moving)
		{
			VuoReal timeSinceLastFrame = time - (*ctx)->timeLastFrame;
			double durationClamped = MAX(duration, 0.00001);
			double timeSinceTargetChanged = MIN(time - (*ctx)->timeWhenTargetChanged, durationClamped);
			double curviness = durationClamped/(3.*timeSinceLastFrame);
			VuoGenericType1 p1 = VuoGenericType1_add((*ctx)->positionWhenTargetChanged, VuoGenericType1_multiply((*ctx)->velocityWhenTargetChanged, timeSinceLastFrame*curviness));
			*position = VuoGenericType1_bezier3((*ctx)->positionWhenTargetChanged, p1, setTarget, setTarget, timeSinceTargetChanged/durationClamped);

			(*ctx)->velocity = VuoGenericType1_divide(VuoGenericType1_subtract(*position, (*ctx)->positionLastFrame), timeSinceLastFrame);
			(*ctx)->positionLastFrame = *position;

			if (time - (*ctx)->timeWhenTargetChanged > durationClamped)
			{
				(*ctx)->moving = false;
				(*ctx)->velocity = VuoGenericType1_valueFromString("");
			}
		}

		(*ctx)->timeLastFrame = time;
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) ctx
)
{
}
