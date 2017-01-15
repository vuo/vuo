/**
 * @file
 * vuo.ui.button.action node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoMouse.h"
#include "VuoImageText.h"
#include "VuoIconPosition.h"

VuoModuleMetadata({
					 "title" : "Make Button",
					 "keywords" : [ "pushed", "command", "selected", "pressed" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ChangeColorWithButton.vuo", "FlipPhoto.vuo" ],
					 },
					 "dependencies": [
						 "VuoMouse",
						 "VuoImageText"
					 ]
				 });

struct nodeInstanceData
{
	VuoText uniqueID;

	VuoMouse *mouseMovedListener;
	VuoMouse *mouseDraggedListener;
	VuoMouse *mousePressedListener;
	VuoMouse *mouseReleasedListener;

	void (*updatedLayer)(VuoLayer layer);
	void (*pressed)(void);

	VuoRenderedLayers renderedLayers;
	dispatch_queue_t renderedLayersQueue;	///< Serializes access to `renderedLayers`.

	bool hovered;
	bool mouseDown;

	VuoText label;
	VuoFont font;
	VuoColor color;
	VuoImage icon;
	VuoIconPosition iconPosition;
	VuoPoint2d center;
	VuoReal width;
	VuoReal height;
};

struct nodeInstanceData *nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);

	context->renderedLayersQueue = dispatch_queue_create("vuo.ui.button.action", NULL);

	context->uniqueID = VuoText_format("%p", context);
	VuoRegister(context->uniqueID, free);
	VuoRetain(context->uniqueID);

	context->mouseMovedListener = VuoMouse_make();
	VuoRetain(context->mouseMovedListener);

	context->mouseDraggedListener = VuoMouse_make();
	VuoRetain(context->mouseDraggedListener);

	context->mousePressedListener = VuoMouse_make();
	VuoRetain(context->mousePressedListener);

	context->mouseReleasedListener = VuoMouse_make();
	VuoRetain(context->mouseReleasedListener);

	return context;
}

static void renderButton(struct nodeInstanceData *context);

static void updateButton(struct nodeInstanceData *context, VuoText label, VuoFont font, VuoColor color, VuoImage icon, VuoIconPosition iconPosition, VuoPoint2d center, VuoReal width, VuoReal height)
{
	bool changed = false;

	if (!VuoText_areEqual(label, context->label))
	{
		VuoRetain(label);
		VuoRelease(context->label);
		context->label = label;
		changed = true;
	}

	if (!VuoFont_areEqual(font, context->font))
	{
		VuoFont_retain(font);
		VuoFont_release(context->font);
		context->font = font;
		changed = true;
	}

	if (!VuoColor_areEqual(color, context->color))
	{
		context->color = color;
		changed = true;
	}

	if (icon != context->icon)
	{
		VuoRetain(icon);
		VuoRelease(context->icon);
		context->icon = icon;
		changed = true;
	}

	if (!VuoIconPosition_areEqual(iconPosition, context->iconPosition))
	{
		context->iconPosition = iconPosition;
		changed = true;
	}

	if (!VuoPoint2d_areEqual(center, context->center))
	{
		context->center = center;
		changed = true;
	}

	if (!VuoReal_areEqual(width, context->width))
	{
		context->width = width;
		changed = true;
	}

	if (!VuoReal_areEqual(height, context->height))
	{
		context->height = height;
		changed = true;
	}

	if (changed)
		renderButton(context);
}

static void renderButton(struct nodeInstanceData *context)
{
	VuoReal h,s,l,a;
	VuoColor_getHSLA(context->color, &h, &s, &l, &a);

	VuoColor backgroundColor = context->color;

	// When the button is pressed, make the background a little darker and more opaque.
	if (context->mouseDown && context->hovered)
		backgroundColor = VuoColor_makeWithHSLA(h, s, MAX(0, l - 0.05), MIN(1, a + 0.05));

	// When hovering, make the background a little brighter and more opaque.
	else if (context->hovered)
		backgroundColor = VuoColor_makeWithHSLA(h, s, MIN(1, l + 0.02), MIN(1, a + 0.02));

	VuoList_VuoLayer layers = VuoListCreate_VuoLayer();
	VuoRetain(layers);

	// Calculate the button's content rectangle.
	// The content rectangle is inset from the edges, at the center of the rounded rectangle's corner circles.
	VuoReal roundness = 0.5;
	VuoRectangle contentRectangle;
	{
		contentRectangle.center = VuoPoint2d_make(0,0);
		VuoReal insetAmount = MIN(context->width, context->height) / 4.;
		contentRectangle.size = VuoPoint2d_make(context->width - 2. * insetAmount, context->height - 2. * insetAmount);
	}

	{
		VuoLayer backgroundLayer = VuoLayer_makeRoundedRectangle(context->uniqueID, backgroundColor, VuoPoint2d_make(0,0), 0, context->width, context->height, 1, roundness);
		VuoListAppendValue_VuoLayer(layers, backgroundLayer);
	}

	bool haveText = VuoText_length(context->label);
	VuoLayer textLayer;
	if (haveText)
		textLayer.sceneObject = VuoSceneObject_makeText(context->label, context->font);

	VuoRectangle iconContainerRectangle = contentRectangle;
	if (context->icon)
	{
		VuoRectangle iconRectangle = VuoImage_getRectangle(context->icon);

		switch (context->iconPosition)
		{
			case VuoIconPosition_Left:
				// Center the label in the right 2/3.
				textLayer.sceneObject.transform.translation = VuoPoint3d_make(contentRectangle.size.x/6., 0, 0);
				// Center the icon in the left 1/3.
				iconContainerRectangle.size.x = contentRectangle.size.x/3.;
				iconContainerRectangle.center.x = -contentRectangle.size.x/3.;
				break;

			case VuoIconPosition_Right:
				// Center the label in the left 2/3.
				textLayer.sceneObject.transform.translation = VuoPoint3d_make(-contentRectangle.size.x/6., 0, 0);
				// Center the icon in the right 1/3.
				iconContainerRectangle.size.x = contentRectangle.size.x/3.;
				iconContainerRectangle.center.x = contentRectangle.size.x/3.;
				break;

			case VuoIconPosition_Above:
				// Center the label in the bottom 2/3.
				textLayer.sceneObject.transform.translation = VuoPoint3d_make(0, -contentRectangle.size.y/6., 0);
				// Center the icon in the top 1/3.
				iconContainerRectangle.size.y = contentRectangle.size.y/3.;
				iconContainerRectangle.center.y = contentRectangle.size.y/3.;
				break;

			case VuoIconPosition_Below:
				// Center the label in the top 2/3.
				textLayer.sceneObject.transform.translation = VuoPoint3d_make(0, contentRectangle.size.y/6., 0);
				// Center the icon in the bottom 1/3.
				iconContainerRectangle.size.y = contentRectangle.size.y/3.;
				iconContainerRectangle.center.y = -contentRectangle.size.y/3.;
				break;

			case VuoIconPosition_Behind:
				// Leave text and icon in current (center) position.
				// Allow the icon to take up the entire contentRectangle.
				break;
		}


		// Adjust iconRectangle's size, preserving aspect ratio, so it fits within contentRectangle.
		if (iconRectangle.size.x > iconContainerRectangle.size.x)
		{
			float scale = iconContainerRectangle.size.x / iconRectangle.size.x;
			iconRectangle.size.x *= scale;
			iconRectangle.size.y *= scale;
		}
		if (iconRectangle.size.y > iconContainerRectangle.size.y)
		{
			float scale = iconContainerRectangle.size.y / iconRectangle.size.y;
			iconRectangle.size.x *= scale;
			iconRectangle.size.y *= scale;
		}
		iconRectangle.center = iconContainerRectangle.center;

		VuoLayer iconLayer = VuoLayer_make(VuoText_make(""), context->icon, iconRectangle.center, 0, iconRectangle.size.x, 1);
		VuoListAppendValue_VuoLayer(layers, iconLayer);
	}

	if (haveText)
		VuoListAppendValue_VuoLayer(layers, textLayer);

//	VuoListAppendValue_VuoLayer(layers, VuoLayer_makeColor(VuoText_make(""), VuoColor_makeWithRGBA(1,0,0,0.5), contentRectangle.center, 0, contentRectangle.size.x, contentRectangle.size.y));
//	VuoListAppendValue_VuoLayer(layers, VuoLayer_makeColor(VuoText_make(""), VuoColor_makeWithRGBA(0,0,1,0.5), iconContainerRectangle.center, 0, iconContainerRectangle.size.x, iconContainerRectangle.size.y));

	VuoLayer group = VuoLayer_makeGroup(layers, VuoTransform2d_make(context->center, 0, VuoPoint2d_make(1,1)));
	VuoRelease(layers);
	context->updatedLayer(group);
}

static void mouseMoved(struct nodeInstanceData *context, VuoPoint2d point)
{
	__block bool hovered;
	dispatch_sync((*context).renderedLayersQueue, ^{
					  hovered = VuoRenderedLayers_isPointInLayer(context->renderedLayers, context->uniqueID, point);
				  });

	if (hovered != context->hovered)
	{
		context->hovered = hovered;
		renderButton(context);
	}
}

static void mousePressed(struct nodeInstanceData *context, VuoPoint2d point)
{
	dispatch_sync((*context).renderedLayersQueue, ^{
					  context->mouseDown = VuoRenderedLayers_isPointInLayer(context->renderedLayers, context->uniqueID, point);
				  });
	renderButton(context);
}

static void mouseReleased(struct nodeInstanceData *context, VuoPoint2d point)
{
	context->mouseDown = false;
	renderButton(context);

	// If the mouse button was _released_ within the layer, the button is considered to be actuated.
	dispatch_sync((*context).renderedLayersQueue, ^{
					  if (VuoRenderedLayers_isPointInLayer(context->renderedLayers, context->uniqueID, point))
						  context->pressed();
				  });
}

static void startTriggers(struct nodeInstanceData *context)
{
	VuoMouse_startListeningForMovesWithCallback(context->mouseMovedListener,
												^(VuoPoint2d point) { mouseMoved(context, point); },
												context->renderedLayers.window, VuoModifierKey_Any);
	VuoMouse_startListeningForDragsWithCallback(context->mouseDraggedListener,
												^(VuoPoint2d point) { mouseMoved(context, point); },
												VuoMouseButton_Left, context->renderedLayers.window, VuoModifierKey_Any, true);
	VuoMouse_startListeningForPressesWithCallback(context->mousePressedListener,
												  ^(VuoPoint2d point) { mousePressed(context, point); },
												  VuoMouseButton_Left, context->renderedLayers.window, VuoModifierKey_Any);
	VuoMouse_startListeningForReleasesWithCallback(context->mouseReleasedListener,
												   ^(VuoPoint2d point) { mouseReleased(context, point); },
												   VuoMouseButton_Left, context->renderedLayers.window, VuoModifierKey_Any, true);
}

static void stopTriggers(struct nodeInstanceData *context)
{
	VuoMouse_stopListening(context->mouseMovedListener);
	VuoMouse_stopListening(context->mouseDraggedListener);
	VuoMouse_stopListening(context->mousePressedListener);
	VuoMouse_stopListening(context->mouseReleasedListener);
}

void nodeInstanceTriggerStart
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) label,
		VuoInputData(VuoFont) font,
		VuoInputData(VuoColor) color,
		VuoInputData(VuoImage) icon,
		VuoInputData(VuoIconPosition) iconPosition,
		VuoInputData(VuoPoint2d) center,
		VuoInputData(VuoReal) width,
		VuoInputData(VuoReal) height,
		VuoOutputTrigger(updatedLayer, VuoLayer),
		VuoOutputTrigger(pressed, void)
)
{
	(*context)->renderedLayers = renderedLayers;
	VuoRenderedLayers_retain((*context)->renderedLayers);

	(*context)->updatedLayer = updatedLayer;
	(*context)->pressed = pressed;

	startTriggers(*context);

	updateButton(*context, label, font, color, icon, iconPosition, center, width, height);
}

void nodeInstanceTriggerUpdate
(
		VuoInstanceData(struct nodeInstanceData *) context,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputData(VuoText) label,
		VuoInputData(VuoFont) font,
		VuoInputData(VuoColor) color,
		VuoInputData(VuoImage) icon,
		VuoInputData(VuoIconPosition) iconPosition,
		VuoInputData(VuoPoint2d) center,
		VuoInputData(VuoReal) width,
		VuoInputData(VuoReal) height
)
{
	bool windowChanged = (*context)->renderedLayers.window != renderedLayers.window;

	dispatch_sync((*context)->renderedLayersQueue, ^{
					  VuoRenderedLayers_release((*context)->renderedLayers);
					  (*context)->renderedLayers = renderedLayers;
					  VuoRenderedLayers_retain((*context)->renderedLayers);
				  });

	if (windowChanged)
	{
		stopTriggers(*context);
		startTriggers(*context);
	}

	updateButton(*context, label, font, color, icon, iconPosition, center, width, height);
}

void nodeInstanceEvent
(
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputEvent({"data":"renderedLayers"}) renderedLayersEvent,
		VuoInputData(VuoText, {"default":"Start"}) label,
		VuoInputData(VuoFont, {"default":{"fontName":"HelveticaNeue-Light","pointSize":28}}) font,
		VuoInputData(VuoColor, {"default":{"r":0.4,"g":0.5,"b":0.6,"a":1.0}}) color,
		VuoInputData(VuoImage) icon,
		VuoInputData(VuoIconPosition, {"default":"left"}) iconPosition,
		VuoInputData(VuoPoint2d, {"suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.4, "suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":0.1, "suggestedStep":0.1}) height,
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	if (renderedLayersEvent)
	{
		bool windowChanged = (*context)->renderedLayers.window != renderedLayers.window;

		dispatch_sync((*context)->renderedLayersQueue, ^{
						  VuoRenderedLayers_release((*context)->renderedLayers);
						  (*context)->renderedLayers = renderedLayers;
						  VuoRenderedLayers_retain((*context)->renderedLayers);
					  });

		if (windowChanged)
		{
			stopTriggers(*context);
			startTriggers(*context);
		}
	}

	updateButton(*context, label, font, color, icon, iconPosition, center, width, height);
}

void nodeInstanceTriggerStop
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	stopTriggers(*context);
	VuoRenderedLayers_release((*context)->renderedLayers);
}

void nodeInstanceFini
(
		VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRelease((*context)->uniqueID);
	VuoRelease((*context)->label);
	VuoFont_release((*context)->font);
	VuoRelease((*context)->icon);
	VuoRelease((*context)->mouseMovedListener);
	VuoRelease((*context)->mouseDraggedListener);
	VuoRelease((*context)->mousePressedListener);
	VuoRelease((*context)->mouseReleasedListener);
	dispatch_release((*context)->renderedLayersQueue);
}
