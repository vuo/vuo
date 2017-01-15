/**
 * @file
 * vuo.midi.bcf2000.foot node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoMidi.h"
#include "VuoRealRegulation.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Foot Controls",
					 "keywords" : [ "controller", "pedal", "footswitch", "Behringer", "BCF-2000", "device" ],
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

	VuoRealRegulation controller[2];
	VuoSmoothInertia controllerSmooth[2];
};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 2; ++i)
		if (controller.controllerNumber == 93+i)
			VuoSmoothInertia_setTarget(context->controllerSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->controller[i].maximumValue - context->controller[i].minimumValue) + context->controller[i].minimumValue);
}

static void resetController(VuoMidiOut outputManager, char cc, VuoRealRegulation controller)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, cc, (controller.defaultValue-controller.minimumValue)/(controller.maximumValue-controller.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) footSwitch,
		VuoInputData(VuoRealRegulation) footController
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

	VuoRealRegulation controllers[2] = {footSwitch, footController};
	for (int i = 0; i < 2; ++i)
	{
		context->controllerSmooth[i] = VuoSmoothInertia_make(controllers[i].defaultValue);
		VuoRetain(context->controllerSmooth[i]);
		// Output the initial position during the first time event.
		context->controllerSmooth[i]->updateNeededAfterSetPosition = true;

		context->controller[i] = controllers[i];
		VuoRealRegulation_retain(context->controller[i]);

		resetController(context->outputManager, 93+i, controllers[i]);
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

		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0, "smoothDuration":0}}) footSwitch,
		VuoInputEvent({"eventBlocking":"wall","data":"footSwitch"}) footSwitchEvent,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) footController,
		VuoInputEvent({"eventBlocking":"wall","data":"footController"}) footControllerEvent,

		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,

		VuoOutputData(VuoReal) footSwitchValue,
		VuoOutputEvent({"data":"footSwitchValue"}) footSwitchValueEvent,
		VuoOutputData(VuoReal) footControllerValue,
		VuoOutputEvent({"data":"footControllerValue"}) footControllerValueEvent
	)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation controllers[2] = {footSwitch,           footController};
	VuoReal     *controllerValues[2] = {footSwitchValue,      footControllerValue};
	bool   *controllerValueEvents[2] = {footSwitchValueEvent, footControllerValueEvent};

	if (reset)
		for (int i = 0; i < 2; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->controllerSmooth[i], controllers[i].defaultValue);
			resetController((*context)->outputManager, 93+i, controllers[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 2; ++i)
		{
			VuoRealRegulation_release((*context)->controller[i]);
			(*context)->controller[i] = controllers[i];
			VuoRealRegulation_retain((*context)->controller[i]);

			VuoSmoothInertia_setDuration((*context)->controllerSmooth[i], controllers[i].smoothDuration);
			*controllerValueEvents[i] = VuoSmoothInertia_step((*context)->controllerSmooth[i], time, controllerValues[i]);
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
	for (int i = 0; i < 2; ++i)
	{
		VuoRelease((*context)->controllerSmooth[i]);
		VuoRealRegulation_release((*context)->controller[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
