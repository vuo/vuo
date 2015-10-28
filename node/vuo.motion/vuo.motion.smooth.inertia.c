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
					  "version" : "1.0.1",
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "CompareSmoothedMotion.vuo", "CompareSmoothedData.vuo" ]
					  }
				 });

#define type VuoGenericType1
#define zeroValue VuoGenericType1_valueFromString("")
#define add VuoGenericType1_add
#define subtract VuoGenericType1_subtract
#define multiply VuoGenericType1_multiply
#define bezier3 VuoGenericType1_bezier3
#include "VuoSmooth.h"

VuoSmoothInertia nodeInstanceInit(
		VuoInputData(VuoGenericType1, {"defaults":{
										   "VuoReal":0.,
										   "VuoPoint2d":{"x":0.,"y":0.},
										   "VuoPoint3d":{"x":0.,"y":0.,"z":0.}
									   }}) setPosition
)
{
	return VuoSmoothInertia_make(setPosition);
}

void nodeInstanceEvent
(
		VuoInstanceData(VuoSmoothInertia) smooth,
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
		VuoOutputEvent({"data":"position"}) positionEvent
)
{
	if (setPositionEvent)
	{
		VuoSmoothInertia_setPosition(*smooth, setPosition);
		*position = setPosition;
	}

	if (setTargetEvent)
		VuoSmoothInertia_setTarget(*smooth, time, setTarget);

	if (timeEvent)
	{
		VuoSmoothInertia_setDuration(*smooth, duration);
		*positionEvent = VuoSmoothInertia_step(*smooth, time, position);
	}
}

void nodeInstanceFini
(
		VuoInstanceData(VuoSmoothInertia) smooth
)
{
}
