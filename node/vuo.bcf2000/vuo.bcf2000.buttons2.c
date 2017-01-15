/**
 * @file
 * vuo.midi.bcf2000.buttons2 node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Buttons Row 2",
					 "keywords" : [ "controller", "Behringer", "BCF-2000", "device" ],
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

	VuoRealRegulation button[8];
	VuoSmoothInertia buttonSmooth[8];
};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 8; ++i)
		if (controller.controllerNumber == 73+i)
			VuoSmoothInertia_setTarget(context->buttonSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->button[i].maximumValue - context->button[i].minimumValue) + context->button[i].minimumValue);
}

static void resetbutton(VuoMidiOut outputManager, char controller, VuoRealRegulation button)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, controller, (button.defaultValue-button.minimumValue)/(button.maximumValue-button.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) button1,
		VuoInputData(VuoRealRegulation) button2,
		VuoInputData(VuoRealRegulation) button3,
		VuoInputData(VuoRealRegulation) button4,
		VuoInputData(VuoRealRegulation) button5,
		VuoInputData(VuoRealRegulation) button6,
		VuoInputData(VuoRealRegulation) button7,
		VuoInputData(VuoRealRegulation) button8
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

	VuoRealRegulation buttons[8] = {button1, button2, button3, button4, button5, button6, button7, button8};
	for (int i = 0; i < 8; ++i)
	{
		context->buttonSmooth[i] = VuoSmoothInertia_make(buttons[i].defaultValue);
		VuoRetain(context->buttonSmooth[i]);
		// Output the initial position during the first time event.
		context->buttonSmooth[i]->updateNeededAfterSetPosition = true;

		context->button[i] = buttons[i];
		VuoRealRegulation_retain(context->button[i]);

		resetbutton(context->outputManager, 73+i, buttons[i]);
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

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button1,
		VuoInputEvent({"eventBlocking":"wall","data":"button1"}) button1Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button2,
		VuoInputEvent({"eventBlocking":"wall","data":"button2"}) button2Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button3,
		VuoInputEvent({"eventBlocking":"wall","data":"button3"}) button3Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button4,
		VuoInputEvent({"eventBlocking":"wall","data":"button4"}) button4Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button5,
		VuoInputEvent({"eventBlocking":"wall","data":"button5"}) button5Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button6,
		VuoInputEvent({"eventBlocking":"wall","data":"button6"}) button6Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button7,
		VuoInputEvent({"eventBlocking":"wall","data":"button7"}) button7Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) button8,
		VuoInputEvent({"eventBlocking":"wall","data":"button8"}) button8Event,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) button1Value,
		VuoOutputEvent({"data":"button1Value"}) button1ValueEvent,
		VuoOutputData(VuoReal) button2Value,
		VuoOutputEvent({"data":"button2Value"}) button2ValueEvent,
		VuoOutputData(VuoReal) button3Value,
		VuoOutputEvent({"data":"button3Value"}) button3ValueEvent,
		VuoOutputData(VuoReal) button4Value,
		VuoOutputEvent({"data":"button4Value"}) button4ValueEvent,
		VuoOutputData(VuoReal) button5Value,
		VuoOutputEvent({"data":"button5Value"}) button5ValueEvent,
		VuoOutputData(VuoReal) button6Value,
		VuoOutputEvent({"data":"button6Value"}) button6ValueEvent,
		VuoOutputData(VuoReal) button7Value,
		VuoOutputEvent({"data":"button7Value"}) button7ValueEvent,
		VuoOutputData(VuoReal) button8Value,
		VuoOutputEvent({"data":"button8Value"}) button8ValueEvent
	)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation buttons[8] = {button1,           button2,           button3,           button4,           button5,           button6,           button7,           button8};
	VuoReal     *buttonValues[8] = {button1Value,      button2Value,      button3Value,      button4Value,      button5Value,      button6Value,      button7Value,      button8Value};
	bool   *buttonValueEvents[8] = {button1ValueEvent, button2ValueEvent, button3ValueEvent, button4ValueEvent, button5ValueEvent, button6ValueEvent, button7ValueEvent, button8ValueEvent};

	if (reset)
		for (int i = 0; i < 8; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->buttonSmooth[i], buttons[i].defaultValue);
			resetbutton((*context)->outputManager, 73+i, buttons[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 8; ++i)
		{
			VuoRealRegulation_release((*context)->button[i]);
			(*context)->button[i] = buttons[i];
			VuoRealRegulation_retain((*context)->button[i]);

			VuoSmoothInertia_setDuration((*context)->buttonSmooth[i], buttons[i].smoothDuration);
			*buttonValueEvents[i] = VuoSmoothInertia_step((*context)->buttonSmooth[i], time, buttonValues[i]);
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
		VuoRelease((*context)->buttonSmooth[i]);
		VuoRealRegulation_release((*context)->button[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
