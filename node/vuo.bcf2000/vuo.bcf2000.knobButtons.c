/**
 * @file
 * vuo.midi.bcf2000.knobButtons node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Knob Buttons",
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

	VuoRealRegulation knobButton[8];
	VuoSmoothInertia knobButtonSmooth[8];
};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 8; ++i)
		if (controller.controllerNumber == 33+i)
			VuoSmoothInertia_setTarget(context->knobButtonSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->knobButton[i].maximumValue - context->knobButton[i].minimumValue) + context->knobButton[i].minimumValue);
}

static void resetknobButton(VuoMidiOut outputManager, char controller, VuoRealRegulation knobButton)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, controller, (knobButton.defaultValue-knobButton.minimumValue)/(knobButton.maximumValue-knobButton.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) knobButton1,
		VuoInputData(VuoRealRegulation) knobButton2,
		VuoInputData(VuoRealRegulation) knobButton3,
		VuoInputData(VuoRealRegulation) knobButton4,
		VuoInputData(VuoRealRegulation) knobButton5,
		VuoInputData(VuoRealRegulation) knobButton6,
		VuoInputData(VuoRealRegulation) knobButton7,
		VuoInputData(VuoRealRegulation) knobButton8
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

	VuoRealRegulation knobButtons[8] = {knobButton1, knobButton2, knobButton3, knobButton4, knobButton5, knobButton6, knobButton7, knobButton8};
	for (int i = 0; i < 8; ++i)
	{
		context->knobButtonSmooth[i] = VuoSmoothInertia_make(knobButtons[i].defaultValue);
		VuoRetain(context->knobButtonSmooth[i]);
		// Output the initial position during the first time event.
		context->knobButtonSmooth[i]->updateNeededAfterSetPosition = true;

		context->knobButton[i] = knobButtons[i];
		VuoRealRegulation_retain(context->knobButton[i]);

		resetknobButton(context->outputManager, 33+i, knobButtons[i]);
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

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton1,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton1"}) knobButton1Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton2,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton2"}) knobButton2Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton3,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton3"}) knobButton3Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton4,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton4"}) knobButton4Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton5,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton5"}) knobButton5Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton6,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton6"}) knobButton6Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton7,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton7"}) knobButton7Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) knobButton8,
		VuoInputEvent({"eventBlocking":"wall","data":"knobButton8"}) knobButton8Event,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) knobButton1Value,
		VuoOutputEvent({"data":"knobButton1Value"}) knobButton1ValueEvent,
		VuoOutputData(VuoReal) knobButton2Value,
		VuoOutputEvent({"data":"knobButton2Value"}) knobButton2ValueEvent,
		VuoOutputData(VuoReal) knobButton3Value,
		VuoOutputEvent({"data":"knobButton3Value"}) knobButton3ValueEvent,
		VuoOutputData(VuoReal) knobButton4Value,
		VuoOutputEvent({"data":"knobButton4Value"}) knobButton4ValueEvent,
		VuoOutputData(VuoReal) knobButton5Value,
		VuoOutputEvent({"data":"knobButton5Value"}) knobButton5ValueEvent,
		VuoOutputData(VuoReal) knobButton6Value,
		VuoOutputEvent({"data":"knobButton6Value"}) knobButton6ValueEvent,
		VuoOutputData(VuoReal) knobButton7Value,
		VuoOutputEvent({"data":"knobButton7Value"}) knobButton7ValueEvent,
		VuoOutputData(VuoReal) knobButton8Value,
		VuoOutputEvent({"data":"knobButton8Value"}) knobButton8ValueEvent
	)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation knobButtons[8] = {knobButton1,           knobButton2,           knobButton3,           knobButton4,           knobButton5,           knobButton6,           knobButton7,           knobButton8};
	VuoReal     *knobButtonValues[8] = {knobButton1Value,      knobButton2Value,      knobButton3Value,      knobButton4Value,      knobButton5Value,      knobButton6Value,      knobButton7Value,      knobButton8Value};
	bool   *knobButtonValueEvents[8] = {knobButton1ValueEvent, knobButton2ValueEvent, knobButton3ValueEvent, knobButton4ValueEvent, knobButton5ValueEvent, knobButton6ValueEvent, knobButton7ValueEvent, knobButton8ValueEvent};

	if (reset)
		for (int i = 0; i < 8; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->knobButtonSmooth[i], knobButtons[i].defaultValue);
			resetknobButton((*context)->outputManager, 33+i, knobButtons[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 8; ++i)
		{
			VuoRealRegulation_release((*context)->knobButton[i]);
			(*context)->knobButton[i] = knobButtons[i];
			VuoRealRegulation_retain((*context)->knobButton[i]);

			VuoSmoothInertia_setDuration((*context)->knobButtonSmooth[i], knobButtons[i].smoothDuration);
			*knobButtonValueEvents[i] = VuoSmoothInertia_step((*context)->knobButtonSmooth[i], time, knobButtonValues[i]);
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
		VuoRelease((*context)->knobButtonSmooth[i]);
		VuoRealRegulation_release((*context)->knobButton[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
