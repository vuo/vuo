/**
 * @file
 * vuo.midi.bcf2000.faders node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoMidi.h"
#include "VuoRealRegulation.h"
#include "VuoScribbleStrip.h"
#include "VuoSmooth_VuoReal.h"

VuoModuleMetadata({
					 "title" : "Receive BCF2000 Faders",
					 "keywords" : [ "controller", "slider", "Behringer", "BCF-2000", "device" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoMidi",
						 "VuoScribbleStrip"
					 ],
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

struct nodeInstanceData
{
	VuoMidiInputDevice inputDevice;
	VuoMidiIn inputManager;

	VuoMidiOutputDevice outputDevice;
	VuoMidiOut outputManager;

	VuoReal mostRecentTime;

	VuoRealRegulation fader[8];
	VuoSmoothInertia faderSmooth[8];
};

static void receivedController(void *ctx, VuoMidiController controller)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)ctx;

	for (int i = 0; i < 8; ++i)
		if (controller.controllerNumber == 81+i)
			VuoSmoothInertia_setTarget(context->faderSmooth[i], context->mostRecentTime, (VuoReal)controller.value/127. * (context->fader[i].maximumValue - context->fader[i].minimumValue) + context->fader[i].minimumValue);
}

static void resetFader(VuoMidiOut outputManager, char controller, VuoRealRegulation fader)
{
	VuoMidiOut_sendController(outputManager, VuoMidiController_make(1, controller, (fader.defaultValue-fader.minimumValue)/(fader.maximumValue-fader.minimumValue) * 127.));
}

struct nodeInstanceData *nodeInstanceInit(
		VuoInputData(VuoRealRegulation) fader1,
		VuoInputData(VuoRealRegulation) fader2,
		VuoInputData(VuoRealRegulation) fader3,
		VuoInputData(VuoRealRegulation) fader4,
		VuoInputData(VuoRealRegulation) fader5,
		VuoInputData(VuoRealRegulation) fader6,
		VuoInputData(VuoRealRegulation) fader7,
		VuoInputData(VuoRealRegulation) fader8
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

	VuoRealRegulation faders[8] = {fader1, fader2, fader3, fader4, fader5, fader6, fader7, fader8};
	for (int i = 0; i < 8; ++i)
	{
		context->faderSmooth[i] = VuoSmoothInertia_make(faders[i].defaultValue);
		VuoRetain(context->faderSmooth[i]);
		// Output the initial position during the first time event.
		context->faderSmooth[i]->updateNeededAfterSetPosition = true;

		context->fader[i] = faders[i];
		VuoRealRegulation_retain(context->fader[i]);

		resetFader(context->outputManager, 81+i, faders[i]);

		VuoScribbleStrip_set(VuoScribbleStrip_Faders, i, faders[i].name);
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
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader1,
		VuoInputEvent({"eventBlocking":"wall","data":"fader1"}) fader1Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader2,
		VuoInputEvent({"eventBlocking":"wall","data":"fader2"}) fader2Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader3,
		VuoInputEvent({"eventBlocking":"wall","data":"fader3"}) fader3Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader4,
		VuoInputEvent({"eventBlocking":"wall","data":"fader4"}) fader4Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader5,
		VuoInputEvent({"eventBlocking":"wall","data":"fader5"}) fader5Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader6,
		VuoInputEvent({"eventBlocking":"wall","data":"fader6"}) fader6Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader7,
		VuoInputEvent({"eventBlocking":"wall","data":"fader7"}) fader7Event,
		VuoInputData(VuoRealRegulation, {"default":{"minimumValue":0, "maximumValue":1, "defaultValue":0.5, "smoothDuration":2}}) fader8,
		VuoInputEvent({"eventBlocking":"wall","data":"fader8"}) fader8Event,
		VuoInputData(VuoReal) time,
		VuoInputEvent({"eventBlocking":"door","data":"time"}) timeEvent,
		VuoInputEvent({"eventBlocking":"wall"}) reset,
		VuoOutputData(VuoReal) fader1Value,
		VuoOutputEvent({"data":"fader1Value"}) fader1ValueEvent,
		VuoOutputData(VuoReal) fader2Value,
		VuoOutputEvent({"data":"fader2Value"}) fader2ValueEvent,
		VuoOutputData(VuoReal) fader3Value,
		VuoOutputEvent({"data":"fader3Value"}) fader3ValueEvent,
		VuoOutputData(VuoReal) fader4Value,
		VuoOutputEvent({"data":"fader4Value"}) fader4ValueEvent,
		VuoOutputData(VuoReal) fader5Value,
		VuoOutputEvent({"data":"fader5Value"}) fader5ValueEvent,
		VuoOutputData(VuoReal) fader6Value,
		VuoOutputEvent({"data":"fader6Value"}) fader6ValueEvent,
		VuoOutputData(VuoReal) fader7Value,
		VuoOutputEvent({"data":"fader7Value"}) fader7ValueEvent,
		VuoOutputData(VuoReal) fader8Value,
		VuoOutputEvent({"data":"fader8Value"}) fader8ValueEvent
)
{
	(*context)->mostRecentTime = time;

	VuoRealRegulation faders[8] = {fader1,           fader2,           fader3,           fader4,           fader5,           fader6,           fader7,           fader8};
	VuoReal     *faderValues[8] = {fader1Value,      fader2Value,      fader3Value,      fader4Value,      fader5Value,      fader6Value,      fader7Value,      fader8Value};
	bool   *faderValueEvents[8] = {fader1ValueEvent, fader2ValueEvent, fader3ValueEvent, fader4ValueEvent, fader5ValueEvent, fader6ValueEvent, fader7ValueEvent, fader8ValueEvent};

	if (reset)
		for (int i = 0; i < 8; ++i)
		{
			VuoSmoothInertia_setPosition((*context)->faderSmooth[i], faders[i].defaultValue);
			resetFader((*context)->outputManager, 81+i, faders[i]);
		}

	// Calculate the next time-step.
	if (timeEvent)
		for (int i = 0; i < 8; ++i)
		{
			VuoRealRegulation_release((*context)->fader[i]);
			(*context)->fader[i] = faders[i];
			VuoRealRegulation_retain((*context)->fader[i]);

			VuoSmoothInertia_setDuration((*context)->faderSmooth[i], faders[i].smoothDuration);
			*faderValueEvents[i] = VuoSmoothInertia_step((*context)->faderSmooth[i], time, faderValues[i]);
		}

	if ((timeEvent && (*context)->mostRecentTime == 0)
	 || reset)
		for (int i = 0; i < 8; ++i)
			VuoScribbleStrip_set(VuoScribbleStrip_Faders, i, faders[i].name);
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
		VuoRelease((*context)->faderSmooth[i]);
		VuoRealRegulation_release((*context)->fader[i]);
	}

	VuoRelease((*context)->outputManager);
	VuoMidiOutputDevice_release((*context)->outputDevice);
	VuoRelease((*context)->inputManager);
	VuoMidiInputDevice_release((*context)->inputDevice);
}
