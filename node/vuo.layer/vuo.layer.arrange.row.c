/**
 * @file
 * vuo.layer.arrange.row node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoList_VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Arrange Layers in Row",
					 "keywords" : [ "line", "place", "layout", "horizontal", "column" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ShowArrangedLayers.vuo", "ShowArrangedTextLayers.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	long pixelsWide, pixelsHigh;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData* instance = (struct nodeInstanceData*) malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);
	instance->pixelsWide = 0;
	instance->pixelsHigh = 0;
	return instance;
}

void nodeInstanceTriggerStart(VuoInstanceData(struct nodeInstanceData *) instance)
{}

void nodeInstanceTriggerUpdate(VuoInstanceData(struct nodeInstanceData *) instance)
{}

void nodeInstanceTriggerStop(VuoInstanceData(struct nodeInstanceData *) instance)
{}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoRenderedLayers) renderedLayers,
		VuoInputEvent({"eventBlocking":"door","data":"renderedLayers"}) renderedLayersEvent,
		VuoInputData(VuoVerticalAlignment, {"default":"center"}) verticalAlignment,
		VuoInputData(VuoAnchor, {"default": {"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) position,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":0.25, "suggestedStep":0.001}) spacing,
		VuoOutputData(VuoLayer) arrangedLayer,
		VuoOutputEvent({"data":"arrangedLayer"}) arrangedLayerEvent
)
{
	if(renderedLayersEvent)
	{
		if((*instance)->pixelsWide != renderedLayers.pixelsWide || (*instance)->pixelsHigh != renderedLayers.pixelsHigh)
		{
			(*instance)->pixelsWide = renderedLayers.pixelsWide;
			(*instance)->pixelsHigh = renderedLayers.pixelsHigh;
		}
		else
		{
			return;
		}
	}

	unsigned int count = VuoListGetCount_VuoLayer(layers);

	VuoList_VuoLayer children = VuoListCreate_VuoLayer();
	VuoRetain(children);

	VuoPoint3d currentPosition = VuoPoint3d_make(0,0,0);
	VuoReal size_y = 0;

	for(int i = 1; i <= count; i++)
	{
		VuoLayer child = VuoListGetValue_VuoLayer(layers, i);

		VuoRectangle bounds;

		// getRect bounds center includes the layer trs
		if(!VuoRenderedLayers_getRect(renderedLayers, child.sceneObject, &bounds))
			continue;

		VuoPoint3d translation = VuoPoint3d_make(
			(currentPosition.x + child.sceneObject.transform.translation.x) + ((bounds.size.x * .5) - bounds.center.x),
			currentPosition.y + child.sceneObject.transform.translation.y,
			0);

		VuoReal offset = 	verticalAlignment == VuoVerticalAlignment_Top ? -bounds.center.y - (bounds.size.y * .5) :
							verticalAlignment == VuoVerticalAlignment_Center ? -bounds.center.y :
							-bounds.center.y + (bounds.size.y * .5);

		translation.y += offset;
		size_y = fmax(size_y, bounds.size.y);

		// translation now set to where each layer is aligned exactly in the row.  applying the original translation allows for tweaks
		translation.x += child.sceneObject.transform.translation.x;
		translation.y += child.sceneObject.transform.translation.y;

		child.sceneObject.transform.translation = translation;

		currentPosition.x += bounds.size.x + spacing;
		VuoListAppendValue_VuoLayer(children, child);
	}

	// apply anchor
	{
		// if horizontal anchor isn't left, offset
		VuoReal width = VuoAnchor_getHorizontal(anchor) != VuoHorizontalAlignment_Left ? currentPosition.x - spacing : 0.;
		VuoReal offsetX = VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Center ? width * .5 : width;

		// if vertical anchor isn't center, offset
		VuoReal center_y =  verticalAlignment == VuoVerticalAlignment_Center ? 0 :
							verticalAlignment == VuoVerticalAlignment_Top ? -size_y * .5 :
							size_y * .5;

		VuoReal offsetY =   VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Center ? -center_y :
							VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Top ? (-center_y - (size_y * .5)) :
							(-center_y + (size_y * .5));

		unsigned long childCount = VuoListGetCount_VuoLayer(children);
		VuoLayer* array = VuoListGetData_VuoLayer(children);

		for(int i = 0; i < childCount; i++)
		{
			array[i].sceneObject.transform.translation.x -= offsetX;
			array[i].sceneObject.transform.translation.y += offsetY;
		}
	}

	*arrangedLayer = VuoLayer_makeGroup(children, VuoTransform2d_make(position, 0, VuoPoint2d_make(1,1)));
	*arrangedLayerEvent = true;

	VuoRelease(children);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
}
