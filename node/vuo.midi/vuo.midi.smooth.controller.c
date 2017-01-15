/**
 * @file
 * vuo.midi.smooth.controller node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidiController.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Filter and Smooth Controller",
					 "keywords" : [ "CC", "custom controller", "effect", "synthesizer", "sequencer", "music", "instrument", "device" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

#define type VuoReal
#define zeroValue 0
#define add VuoReal_add
#define subtract VuoReal_subtract
#define multiply VuoReal_multiply
#define bezier3 VuoReal_bezier3
#include "VuoSmooth.h"

struct nodeInstanceData
{
	VuoSmoothInertia smooth;
};

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) smoothing
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->smooth = VuoSmoothInertia_make(smoothing.defaultValue);
	VuoRetain(context->smooth);
	// Output the initial position during the first time event.
	context->smooth->updateNeededAfterSetPosition = true;

	return context;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,

		VuoInputData(VuoMidiController) controller,
		VuoInputEvent({"eventBlocking":"wall","data":"controller"}) controllerEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":1, "suggestedMax":16}) channel,
		VuoInputEvent({"eventBlocking":"wall", "data":"channel"}) channelEvent,

		VuoInputData(VuoInteger, {"default":1, "suggestedMin":0, "suggestedMax":127}) controllerNumber,
		VuoInputEvent({"eventBlocking":"wall", "data":"controllerNumber"}) controllerNumberEvent,

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) smoothing,
		VuoInputEvent({"eventBlocking":"wall","data":"smoothing"}) smoothingEvent,

		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) value,
		VuoOutputEvent({"data":"value"}) valueEvent
)
{
	if (controllerEvent && controller.channel == channel && controller.controllerNumber == controllerNumber)
		VuoSmoothInertia_setTarget((*context)->smooth, time, (VuoReal)controller.value/127. * (smoothing.maximumValue - smoothing.minimumValue) + smoothing.minimumValue);

	if (reset)
		VuoSmoothInertia_setPosition((*context)->smooth, smoothing.defaultValue);

	// Calculate the next time-step.
	if (timeEvent)
	{
		VuoSmoothInertia_setDuration((*context)->smooth, smoothing.smoothDuration);
		*valueEvent = VuoSmoothInertia_step((*context)->smooth, time, value);
	}
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->smooth);
}
