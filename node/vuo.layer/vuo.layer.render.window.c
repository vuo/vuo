/**
 * @file
 * vuo.layer.render.window node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoCompositionState.h"
#include "VuoWindow.h"
#include "VuoSceneRenderer.h"
#include "VuoLayer.h"
#include "VuoList_VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoMouse.h"
#include "VuoEventLoop.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Render Layers to Window",
					 "keywords" : [
						 "draw", "graphics",
						 "2D",
						 "display", "view", "screen", "full screen", "fullscreen",
					 ],
					 "version" : "2.4.0",
					 "dependencies" : [
						 "VuoSceneRenderer",
						 "VuoWindow",
						 "VuoMouse"
					 ],
					 "node": {
						 "isDeprecated": true,
						 "exampleCompositions" : [ "DisplayImagesOnLayers.vuo", "RotateGears.vuo" ]
					 }
				 });


struct nodeInstanceData
{
	VuoWindowOpenGl *window;
	VuoSceneRenderer *sceneRenderer;
	VuoMultisample multisampling;

	VuoList_VuoInteraction interactions;
	dispatch_queue_t sceneRendererQueue;

	int windowWidth;
	int windowHeight;
	float backingScaleFactor;

	// Default interaction (used when no Make Interaction nodes are
	// attached to window properties)
	VuoInteraction defaultInteraction;
	// Defines if there are any property interactions
	bool hasWindowPropertyInteractions;

	VuoMouse* mouseMovedListener;
	VuoMouse* mousePressedListener;
	VuoMouse* mouseReleasedListener;
};

static void vuo_layer_render_window_init(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
	VuoRetain(context->sceneRenderer);
}

static void vuo_layer_render_window_updateBacking(void *ctx, float backingScaleFactor)
{
	struct nodeInstanceData *context = ctx;

	bool valid = false;
	VuoSceneObject so = VuoSceneRenderer_getRootSceneObject(context->sceneRenderer, &valid);
	if (valid)
		VuoSceneObject_retain(so);

	dispatch_sync(context->sceneRendererQueue, ^{
		VuoRelease(context->sceneRenderer);

		context->sceneRenderer = VuoSceneRenderer_make(backingScaleFactor);
		VuoRetain(context->sceneRenderer);
	});

	if (valid)
	{
		VuoSceneRenderer_setRootSceneObject(context->sceneRenderer, so);
		VuoSceneObject_release(so);
	}
}

static void vuo_layer_render_window_resize(void *ctx, unsigned int width, unsigned int height)
{
	struct nodeInstanceData *context = ctx;
	dispatch_sync(context->sceneRendererQueue, ^{
		VuoSceneRenderer_regenerateProjectionMatrix(context->sceneRenderer, width, height);
	});
}

static VuoIoSurface vuo_layer_render_window_draw(void *ctx)
{
	struct nodeInstanceData *context = ctx;
	__block VuoIoSurface vis;
	dispatch_sync(context->sceneRendererQueue, ^{
		vis = VuoSceneRenderer_renderToIOSurface(context->sceneRenderer, VuoImageColorDepth_8, context->multisampling, false);
	});
	return vis;
}

static int indexOfUuid(VuoList_VuoInteraction list, const VuoUuid uuid)
{
	for(int i = 1; i <= VuoListGetCount_VuoInteraction(list); i++)
		if( VuoUuid_areEqual(VuoListGetValue_VuoInteraction(list, i).uuid, uuid) )
			return i;
	return -1;
}

/**
 * Get a list of the interactions that have changed since the last frame, including new ones.
 */
static VuoList_VuoInteraction getModifiedInteractions(struct nodeInstanceData* instance, VuoList_VuoWindowProperty properties)
{
	VuoList_VuoInteraction previous = instance->interactions;
	VuoList_VuoInteraction interactions = VuoListCreate_VuoInteraction();

	for(int i = 1; i <= VuoListGetCount_VuoWindowProperty(properties); i++)
	{
		VuoWindowProperty property = VuoListGetValue_VuoWindowProperty(properties, i);

		if(property.type == VuoWindowProperty_Interaction)
		{
			// uuid exists, check that if it is different
			int index = indexOfUuid(previous, property.interaction.uuid);

			if( index > 0 )
			{
				VuoInteraction previousInteraction = VuoListGetValue_VuoInteraction(previous, index);

				// if it is different, pass it on through.  otherwise, don't care.
				if( !VuoInteraction_areEqual(previousInteraction, property.interaction) )
				{
					VuoListSetValue_VuoInteraction(previous, property.interaction, index, false);
					VuoListAppendValue_VuoInteraction(interactions, property.interaction);
				}
			}
			else
			{
				// this is a new interaction, add it to the list of existing interactions and pass it on
				VuoListAppendValue_VuoInteraction(previous, property.interaction);
				VuoListAppendValue_VuoInteraction(interactions, property.interaction);
			}
		}
	}

	return interactions;
}

struct nodeInstanceData *nodeInstanceInit(void)
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->sceneRenderer = NULL;
	context->sceneRendererQueue = dispatch_queue_create("org.vuo.scene.render.window.sceneRenderer", VuoEventLoop_getDispatchInteractiveAttribute());

	context->interactions = VuoListCreate_VuoInteraction();
	VuoRetain(context->interactions);

	context->window = VuoWindowOpenGl_make(
				vuo_layer_render_window_init,
				vuo_layer_render_window_updateBacking,
				vuo_layer_render_window_resize,
				vuo_layer_render_window_draw,
				(void *)context
			);
	VuoRetain(context->window);

	context->mouseMovedListener = VuoMouse_make();
	context->mousePressedListener = VuoMouse_make();
	context->mouseReleasedListener = VuoMouse_make();
	VuoRetain(context->mouseMovedListener);
	VuoRetain(context->mousePressedListener);
	VuoRetain(context->mouseReleasedListener);

	context->defaultInteraction = VuoInteraction_make();
	context->hasWindowPropertyInteractions = false;

	return context;
}

static void onDefaultInteractionChanged(struct nodeInstanceData* context, VuoWindowReference windowReference, void (*renderedLayers)(VuoRenderedLayers))
{
	dispatch_sync(context->sceneRendererQueue,
	^{
		if(context->hasWindowPropertyInteractions)
			return;

		VuoList_VuoInteraction modifiedInteractions = VuoListCreate_VuoInteraction();
		VuoListAppendValue_VuoInteraction(modifiedInteractions, context->defaultInteraction);

		bool valid = false;
		VuoSceneObject so = VuoSceneRenderer_getRootSceneObject(context->sceneRenderer, &valid);
		if (valid)
			VuoSceneObject_retain(so);

		renderedLayers(
			VuoRenderedLayers_makeWithWindow(
				valid ? so : NULL,
				context->windowWidth,
				context->windowHeight,
				context->backingScaleFactor,
				windowReference,
				modifiedInteractions ));

		if (valid)
			VuoSceneObject_release(so);
	});
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal),
		VuoOutputTrigger(renderedLayers, VuoRenderedLayers)
)
{
	VuoWindowOpenGl_enableTriggers_deprecated((*context)->window, showedWindow, requestedFrame);

	VuoWindowReference windowReference = VuoWindowReference_make((*context)->window);
	VuoInteraction* interactionPtr = &(*context)->defaultInteraction;

	VuoMouse_startListeningForMovesWithCallback(
		(*context)->mouseMovedListener,
		^(VuoPoint2d point)
		{
			if(VuoInteraction_update(point, interactionPtr->isPressed, interactionPtr))
				onDefaultInteractionChanged(*context, windowReference, renderedLayers);
		},
		windowReference,
		VuoModifierKey_Any);

	VuoMouse_startListeningForPressesWithCallback(
		(*context)->mousePressedListener,
		^(VuoPoint2d point)
		{
			if(VuoInteraction_update(point, true, interactionPtr))
				onDefaultInteractionChanged(*context, windowReference, renderedLayers);
		},
		NULL,
		VuoMouseButton_Any,
		windowReference,
		VuoModifierKey_Any
		);

	VuoMouse_startListeningForReleasesWithCallback(
		(*context)->mouseReleasedListener,
		^(VuoPoint2d point)
		{
			if(VuoInteraction_update(point, false, interactionPtr))
				onDefaultInteractionChanged(*context, windowReference, renderedLayers);
		},
		VuoMouseButton_Any,
		windowReference,
		VuoModifierKey_Any,
		false
		);
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoMultisample, {"default":"4"}) multisampling,
		VuoInputData(VuoList_VuoWindowProperty) setWindowProperties,
		VuoInputEvent({"eventBlocking":"none","data":"setWindowProperties"}) setWindowPropertiesEvent,
		VuoOutputTrigger(showedWindow, VuoWindowReference),
		VuoOutputTrigger(requestedFrame, VuoReal, {"name":"Refreshed at Time", "eventThrottling":"drop"}),
		VuoOutputTrigger(renderedLayers, VuoRenderedLayers)
)
{
	(*context)->multisampling = multisampling;

	if (setWindowPropertiesEvent)
		VuoWindowOpenGl_setProperties((*context)->window, setWindowProperties);

	VuoSceneObject rootSceneObject = (VuoSceneObject)VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity());

	dispatch_sync((*context)->sceneRendererQueue, ^{
		VuoSceneRenderer_setRootSceneObject((*context)->sceneRenderer, rootSceneObject);
	});

	// Schedule a redraw.
	VuoWindowOpenGl_redraw((*context)->window);

	VuoInteger width, height;
	float backingScaleFactor;
	VuoWindowReference_getContentSize(VuoWindowReference_make((*context)->window), &width, &height, &backingScaleFactor);

	VuoList_VuoInteraction modifiedInteractions = NULL;
	VuoList_VuoWindowProperty windowPropertyInteractions = VuoWindowProperty_getPropertiesWithType(setWindowProperties, VuoWindowProperty_Interaction);
	VuoLocal(windowPropertyInteractions);

	if( VuoListGetCount_VuoWindowProperty(windowPropertyInteractions) > 0 )
	{
		modifiedInteractions = getModifiedInteractions(*context, setWindowProperties);
		(*context)->hasWindowPropertyInteractions = true;
	}

	(*context)->windowWidth = width;
	(*context)->windowHeight = height;
	(*context)->backingScaleFactor = backingScaleFactor;

	renderedLayers(VuoRenderedLayers_makeWithWindow(rootSceneObject, width, height, backingScaleFactor, VuoWindowReference_make((*context)->window), modifiedInteractions));
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoMouse_stopListening((*context)->mousePressedListener);
	VuoMouse_stopListening((*context)->mouseReleasedListener);
	VuoMouse_stopListening((*context)->mouseMovedListener);

	VuoWindowOpenGl_disableTriggers((*context)->window);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	struct nodeInstanceData *c = *context;

	// Ensure the context isn't deallocated until the window has finished closing
	// (which might be a while if macOS is doing its exit-fullscreen animation).
	VuoRetain(c);

	VuoRelease(c->mousePressedListener);
	VuoRelease(c->mouseReleasedListener);
	VuoRelease(c->mouseMovedListener);

	VuoWindowOpenGl_close(c->window, ^{
		VuoRelease(c->interactions);
		VuoRelease(c->sceneRenderer);
		dispatch_release(c->sceneRendererQueue);
		VuoRelease(c);
	});

	VuoRelease(c->window);
}
