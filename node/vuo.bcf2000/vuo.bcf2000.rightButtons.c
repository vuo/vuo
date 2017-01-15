/**
 * @file
 * vuo.midi.bcf2000.rightButtons node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Right Buttons",
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

	VuoRealRegulation button[10];
	VuoSmoothInertia buttonSmooth[10];
};

static char controllerNumbers[10] = {95, 96, 97, 98, 99, 100, 89, 90, 91, 92};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 10; ++i)
		if (controller.controllerNumber == controllerNumbers[i])
			VuoSmoothInertia_setTarget(context->buttonSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->button[i].maximumValue - context->button[i].minimumValue) + context->button[i].minimumValue);
}

static void resetbutton(VuoMidiOut outputManager, char controller, VuoRealRegulation button)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, controller, (button.defaultValue-button.minimumValue)/(button.maximumValue-button.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) storeButton,
		VuoInputData(VuoRealRegulation) learnButton,
		VuoInputData(VuoRealRegulation) editButton,
		VuoInputData(VuoRealRegulation) exitButton,
		VuoInputData(VuoRealRegulation) leftButton,
		VuoInputData(VuoRealRegulation) rightButton,
		VuoInputData(VuoRealRegulation) buttonA,
		VuoInputData(VuoRealRegulation) buttonB,
		VuoInputData(VuoRealRegulation) buttonC,
		VuoInputData(VuoRealRegulation) buttonD
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

	VuoRealRegulation buttons[10] = {storeButton, learnButton, editButton, exitButton, leftButton, rightButton, buttonA, buttonB, buttonC, buttonD};
	for (int i = 0; i < 10; ++i)
	{
		context->buttonSmooth[i] = VuoSmoothInertia_make(buttons[i].defaultValue);
		VuoRetain(context->buttonSmooth[i]);
		// Output the initial position during the first time event.
		context->buttonSmooth[i]->updateNeededAfterSetPosition = true;

		context->button[i] = buttons[i];
		VuoRealRegulation_retain(context->button[i]);

		resetbutton(context->outputManager, controllerNumbers[i], buttons[i]);
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

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) storeButton,
		VuoInputEvent({"eventBlocking":"wall","data":"storeButton"}) storeButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) learnButton,
		VuoInputEvent({"eventBlocking":"wall","data":"learnButton"}) learnButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) editButton,
		VuoInputEvent({"eventBlocking":"wall","data":"editButton"}) editButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) exitButton,
		VuoInputEvent({"eventBlocking":"wall","data":"exitButton"}) exitButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) leftButton,
		VuoInputEvent({"eventBlocking":"wall","data":"leftButton"}) leftButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) rightButton,
		VuoInputEvent({"eventBlocking":"wall","data":"rightButton"}) rightButtonEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) buttonA,
		VuoInputEvent({"eventBlocking":"wall","data":"buttonA"}) buttonAEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) buttonB,
		VuoInputEvent({"eventBlocking":"wall","data":"buttonB"}) buttonBEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) buttonC,
		VuoInputEvent({"eventBlocking":"wall","data":"buttonC"}) buttonCEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) buttonD,
		VuoInputEvent({"eventBlocking":"wall","data":"buttonD"}) buttonDEvent,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) storeButtonValue,
		VuoOutputEvent({"data":"storeButtonValue"}) storeButtonValueEvent,
		VuoOutputData(VuoReal) learnButtonValue,
		VuoOutputEvent({"data":"learnButtonValue"}) learnButtonValueEvent,
		VuoOutputData(VuoReal) editButtonValue,
		VuoOutputEvent({"data":"editButtonValue"}) editButtonValueEvent,
		VuoOutputData(VuoReal) exitButtonValue,
		VuoOutputEvent({"data":"exitButtonValue"}) exitButtonValueEvent,
		VuoOutputData(VuoReal) leftButtonValue,
		VuoOutputEvent({"data":"leftButtonValue"}) leftButtonValueEvent,
		VuoOutputData(VuoReal) rightButtonValue,
		VuoOutputEvent({"data":"rightButtonValue"}) rightButtonValueEvent,
		VuoOutputData(VuoReal) buttonAValue,
		VuoOutputEvent({"data":"buttonAValue"}) buttonAValueEvent,
		VuoOutputData(VuoReal) buttonBValue,
		VuoOutputEvent({"data":"buttonBValue"}) buttonBValueEvent,
		VuoOutputData(VuoReal) buttonCValue,
		VuoOutputEvent({"data":"buttonCValue"}) buttonCValueEvent,
		VuoOutputData(VuoReal) buttonDValue,
		VuoOutputEvent({"data":"buttonDValue"}) buttonDValueEvent
	)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation buttons[10] = {storeButton,           learnButton,           editButton,           exitButton,           leftButton,           rightButton,           buttonA,           buttonB,           buttonC,           buttonD};
	VuoReal     *buttonValues[10] = {storeButtonValue,      learnButtonValue,      editButtonValue,      exitButtonValue,      leftButtonValue,      rightButtonValue,      buttonAValue,      buttonBValue,      buttonCValue,      buttonDValue};
	bool   *buttonValueEvents[10] = {storeButtonValueEvent, learnButtonValueEvent, editButtonValueEvent, exitButtonValueEvent, leftButtonValueEvent, rightButtonValueEvent, buttonAValueEvent, buttonBValueEvent, buttonCValueEvent, buttonDValueEvent};

	if (reset)
		for (int i = 0; i < 10; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->buttonSmooth[i], buttons[i].defaultValue);
			resetbutton((*context)->outputManager, controllerNumbers[i], buttons[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 10; ++i)
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
	for (int i = 0; i < 10; ++i)
	{
		VuoRelease((*context)->buttonSmooth[i]);
		VuoRealRegulation_release((*context)->button[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
