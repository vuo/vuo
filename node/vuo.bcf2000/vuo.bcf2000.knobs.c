/**
 * @file
 * vuo.midi.bcf2000.knobs node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Knobs",
					 "keywords" : [ "controller", "encoders", "dials", "Behringer", "BCF-2000", "device" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoMidi"
					 ],
					 "node": {
						 "isInterface" : true,
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
	VuoMidiInputDevice inputDevice;
	VuoMidiIn inputManager;

	VuoMidiOutputDevice outputDevice;
	VuoMidiOut outputManager;

	VuoReal mostRecentTime;

	VuoRealRegulation knob[8];
	VuoSmoothInertia knobSmooth[8];
};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 8; ++i)
		if (controller.controllerNumber == 1+i)
			VuoSmoothInertia_setTarget(context->knobSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->knob[i].maximumValue - context->knob[i].minimumValue) + context->knob[i].minimumValue);
}

static void resetKnob(VuoMidiOut outputManager, char controller, VuoRealRegulation knob)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, controller, (knob.defaultValue-knob.minimumValue)/(knob.maximumValue-knob.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) knob1,
		VuoInputData(VuoRealRegulation) knob2,
		VuoInputData(VuoRealRegulation) knob3,
		VuoInputData(VuoRealRegulation) knob4,
		VuoInputData(VuoRealRegulation) knob5,
		VuoInputData(VuoRealRegulation) knob6,
		VuoInputData(VuoRealRegulation) knob7,
		VuoInputData(VuoRealRegulation) knob8
)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->inputDevice = VuoMidiInputDevice_make(-1, VuoText_make("BCF2000"));
	VuoMidiInputDevice_retain(context->inputDevice);
	context->inputManager = VuoMidiIn_make(context->inputDevice);
	VuoRetain(context->inputManager);

	context->outputDevice = VuoMidiOutputDevice_make(-1, VuoText_make("BCF2000"));
	VuoMidiOutputDevice_retain(context->outputDevice);
	context->outputManager = VuoMidiOut_make(context->outputDevice);
	VuoRetain(context->outputManager);

	VuoRealRegulation knobs[8] = {knob1, knob2, knob3, knob4, knob5, knob6, knob7, knob8};
	for (int i = 0; i < 8; ++i)
	{
		context->knobSmooth[i] = VuoSmoothInertia_make(knobs[i].defaultValue);
		VuoRetain(context->knobSmooth[i]);
		// Output the initial position during the first time event.
		context->knobSmooth[i]->updateNeededAfterSetPosition = true;

		context->knob[i] = knobs[i];
		VuoRealRegulation_retain(context->knob[i]);

		resetKnob(context->outputManager, 1+i, knobs[i]);
	}

	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiIn_enableTriggers((*context)->inputManager, NULL, receivedController, NULL, *context);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob1,
		VuoInputEvent({"eventBlocking":"wall","data":"knob1"}) knob1Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob2,
		VuoInputEvent({"eventBlocking":"wall","data":"knob2"}) knob2Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob3,
		VuoInputEvent({"eventBlocking":"wall","data":"knob3"}) knob3Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob4,
		VuoInputEvent({"eventBlocking":"wall","data":"knob4"}) knob4Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob5,
		VuoInputEvent({"eventBlocking":"wall","data":"knob5"}) knob5Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob6,
		VuoInputEvent({"eventBlocking":"wall","data":"knob6"}) knob6Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob7,
		VuoInputEvent({"eventBlocking":"wall","data":"knob7"}) knob7Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) knob8,
		VuoInputEvent({"eventBlocking":"wall","data":"knob8"}) knob8Event,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) knob1Value,
		VuoOutputEvent({"data":"knob1Value"}) knob1ValueEvent,
		VuoOutputData(VuoReal) knob2Value,
		VuoOutputEvent({"data":"knob2Value"}) knob2ValueEvent,
		VuoOutputData(VuoReal) knob3Value,
		VuoOutputEvent({"data":"knob3Value"}) knob3ValueEvent,
		VuoOutputData(VuoReal) knob4Value,
		VuoOutputEvent({"data":"knob4Value"}) knob4ValueEvent,
		VuoOutputData(VuoReal) knob5Value,
		VuoOutputEvent({"data":"knob5Value"}) knob5ValueEvent,
		VuoOutputData(VuoReal) knob6Value,
		VuoOutputEvent({"data":"knob6Value"}) knob6ValueEvent,
		VuoOutputData(VuoReal) knob7Value,
		VuoOutputEvent({"data":"knob7Value"}) knob7ValueEvent,
		VuoOutputData(VuoReal) knob8Value,
		VuoOutputEvent({"data":"knob8Value"}) knob8ValueEvent
	)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation knobs[8] = {knob1,           knob2,           knob3,           knob4,           knob5,           knob6,           knob7,           knob8};
	VuoReal     *knobValues[8] = {knob1Value,      knob2Value,      knob3Value,      knob4Value,      knob5Value,      knob6Value,      knob7Value,      knob8Value};
	bool   *knobValueEvents[8] = {knob1ValueEvent, knob2ValueEvent, knob3ValueEvent, knob4ValueEvent, knob5ValueEvent, knob6ValueEvent, knob7ValueEvent, knob8ValueEvent};

	if (reset)
		for (int i = 0; i < 8; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->knobSmooth[i], knobs[i].defaultValue);
			resetKnob((*context)->outputManager, 1+i, knobs[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 8; ++i)
		{
			VuoRealRegulation_release((*context)->knob[i]);
			(*context)->knob[i] = knobs[i];
			VuoRealRegulation_retain((*context)->knob[i]);

			VuoSmoothInertia_setDuration((*context)->knobSmooth[i], knobs[i].smoothDuration);
			*knobValueEvents[i] = VuoSmoothInertia_step((*context)->knobSmooth[i], time, knobValues[i]);
		}
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMidiIn_disableTriggers((*context)->inputManager);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	for (int i = 0; i < 8; ++i)
	{
		VuoRelease((*context)->knobSmooth[i]);
		VuoRealRegulation_release((*context)->knob[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
