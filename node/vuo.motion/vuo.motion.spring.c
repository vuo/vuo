/**
 * @file
 * vuo.motion.spring node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <math.h>

VuoModuleMetadata({
					  "title" : "Spring",
					  "keywords" : [ "bounce", "oscillate", "rebound", "ricochet", "wobble", "harmonic", "sine", "sinusoidial" ],
					  "version" : "1.0.0",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
				 });

struct nodeInstanceData
{
	bool first;
	bool updateNeededAfterSetPosition;
	VuoReal dropTime;
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
	VuoInputEvent(VuoPortEventBlocking_Door, time) timeEvent,
	VuoInputData(VuoGenericType1) setRestingPosition,
	VuoInputEvent(VuoPortEventBlocking_Wall, setRestingPosition) setRestingPositionEvent,
	VuoInputData(VuoGenericType1, {"defaults":{"VuoReal":1., "VuoPoint2d":{"x":0.,"y":1.}, "VuoPoint3d":{"x":0.,"y":1.,"z":0.}}}) dropFromPosition,
	VuoInputEvent(VuoPortEventBlocking_Wall, dropFromPosition) dropFromPositionEvent,
	VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0.000001, "suggestedStep":0.1}) period,
	VuoInputEvent(VuoPortEventBlocking_Wall, period) periodEvent,
	VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) damping,
	VuoInputEvent(VuoPortEventBlocking_Wall, damping) dampingEvent,
	VuoOutputData(VuoGenericType1) position,
	VuoOutputEvent(position) positionEvent,
	VuoInstanceData(struct nodeInstanceData *) ctx
)
{
	if ((*ctx)->first || setRestingPositionEvent)
	{
		*position = setRestingPosition;
		(*ctx)->dropTime = NAN;
		(*ctx)->updateNeededAfterSetPosition = true;
		(*ctx)->first = false;
	}

	double positivePeriod = MAX(period,0.000001);
	double clampedDamping = MAX(MIN(damping,0.999999),0.000001);

	if (dropFromPositionEvent)
		(*ctx)->dropTime = time;

	bool springSimulationRunning = !isnan((*ctx)->dropTime);
	if (timeEvent && (springSimulationRunning || (*ctx)->updateNeededAfterSetPosition))
	{
		*positionEvent = true;
		(*ctx)->updateNeededAfterSetPosition = false;
	}

	if (timeEvent && springSimulationRunning)
	{
		double timeSinceDrop = time - (*ctx)->dropTime;
		*position = VuoGenericType1_spring(timeSinceDrop, dropFromPosition, setRestingPosition, period, clampedDamping);

		if (timeSinceDrop > 2.*period/clampedDamping)
		{
			// When the spring has slowed down, bring the spring home, and stop the simulation (and stop passing events).
			*position = setRestingPosition;
			(*ctx)->dropTime = NAN;
		}
	}
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) ctx)
{
}
