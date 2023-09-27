/**
 * @file
 * vuo.screen.capture node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#import "VuoScreenCapture.h"
#import "VuoScreenCommon.h"

VuoModuleMetadata({
					 "title" : "Capture Image of Screen",
					 "keywords" : [ "receive", "screenshot", "window", "grab", "record", "stream", "rectangle" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ],
					 },
					 "dependencies" : [
						 "VuoRectangle",
						 "VuoScreenCapture",
						 "VuoScreenCommon"
					 ]
				 });


struct nodeInstanceData
{
	VuoScreenCapture capture;

	VuoScreen screen;
	VuoRectangle rectangle;

	bool triggersEnabled;
};

static void initialize(struct nodeInstanceData *context, VuoScreen screen, VuoRectangle rectangle, VuoOutputTrigger(capturedImage, VuoImage))
{
	VuoScreen realizedScreen;
	if (!VuoScreen_realize(screen, &realizedScreen))
		realizedScreen = VuoScreen_getSecondary();
	VuoScreen_retain(realizedScreen);

	if (VuoScreen_areEqual(realizedScreen, context->screen) && VuoRectangle_areEqual(rectangle, context->rectangle))
	{
		VuoScreen_release(realizedScreen);
		return;
	}

	VuoScreen_release(context->screen);
	context->screen = realizedScreen;

	context->rectangle = rectangle;

	VuoRelease(context->capture);
	context->capture = VuoScreenCapture_make(context->screen, context->rectangle, capturedImage);
	VuoRetain(context->capture);
}

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1, sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	return context;
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoScreen) screen,
		VuoInputData(VuoPoint2d) topLeft,
		VuoInputData(VuoInteger) width,
		VuoInputData(VuoInteger) height,
		VuoOutputTrigger(capturedImage, VuoImage)
)
{
	(*context)->triggersEnabled = true;
	initialize(*context, screen, VuoRectangle_makeTopLeft(topLeft.x, topLeft.y, width, height), capturedImage);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoScreen) screen,
		VuoInputData(VuoPoint2d) topLeft,
		VuoInputData(VuoInteger) width,
		VuoInputData(VuoInteger) height,
		VuoOutputTrigger(capturedImage, VuoImage)
)
{
	initialize(*context, screen, VuoRectangle_makeTopLeft(topLeft.x, topLeft.y, width, height), capturedImage);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoScreen, {"default":{"type":"secondary"}}) screen,
		VuoInputData(VuoPoint2d, {"suggestedMin":"0,0"}) topLeft,
		VuoInputData(VuoInteger, {"default":640, "suggestedMin":1}) width,
		VuoInputData(VuoInteger, {"default":480, "suggestedMin":1}) height,
		VuoOutputTrigger(capturedImage, VuoImage, {"eventThrottling":"drop"})
)
{
	if (!(*context)->triggersEnabled)
		return;

	initialize(*context, screen, VuoRectangle_makeTopLeft(topLeft.x, topLeft.y, width, height), capturedImage);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->capture);
	VuoScreen_release((*context)->screen);
	(*context)->triggersEnabled = false;
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
}
