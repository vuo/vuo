/**
 * @file
 * vuo.layer.arrange.column node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoList_VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Arrange Layers in Column",
					 "keywords" : [ "line", "place", "layout", "vertical", "row" ],
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
		VuoInputData(VuoHorizontalAlignment, {"default":"center"}) horizontalAlignment,
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
	VuoReal size_x = 0;

	for(int i = 1; i <= count; i++)
	{
		VuoLayer child = VuoListGetValue_VuoLayer(layers, i);

		VuoRectangle bounds;

		// getRect bounds center includes the layer trs
		if(!VuoRenderedLayers_getRect(renderedLayers, child, &bounds))
			continue;

		size_x = fmax(size_x, bounds.size.x);

		VuoPoint3d translation = VuoPoint3d_make(
			currentPosition.x + child.sceneObject.transform.translation.x,
			(currentPosition.y + child.sceneObject.transform.translation.y) - ((bounds.size.y * .5) + bounds.center.y),
			0);

		if(horizontalAlignment == VuoHorizontalAlignment_Right)
			translation.x += -bounds.center.x - (bounds.size.x * .5);
		else if(horizontalAlignment == VuoHorizontalAlignment_Center)
			translation.x += -bounds.center.x;
		else
			translation.x += -bounds.center.x + (bounds.size.x * .5);

		// translation now set to where each layer is aligned exactly in the row.  applying the original translation allows for tweaks
		translation.x += child.sceneObject.transform.translation.x;
		translation.y += child.sceneObject.transform.translation.y;

		child.sceneObject.transform.translation = translation;

		currentPosition.y -= bounds.size.y + spacing;
		VuoListAppendValue_VuoLayer(children, child);
	}

	{
		VuoReal center_x = 	horizontalAlignment == VuoHorizontalAlignment_Center ? 0 :
							horizontalAlignment == VuoHorizontalAlignment_Left ? size_x * .5 :
							size_x * -.5;

		VuoReal offset_x = 	anchor.horizontalAlignment == VuoHorizontalAlignment_Left ? (-center_x + (size_x * .5)) :
							anchor.horizontalAlignment == VuoHorizontalAlignment_Center ? -center_x :
							(-center_x - (size_x * .5));

		VuoReal height = anchor.verticalAlignment == VuoVerticalAlignment_Top ? 0 : fabs(currentPosition.y) - spacing;
		VuoReal offset_y = anchor.verticalAlignment == VuoVerticalAlignment_Center ? height * .5 : height;

		unsigned long childCount = VuoListGetCount_VuoLayer(children);
		VuoLayer* array = VuoListGetData_VuoLayer(children);

		for(int i = 0; i < childCount; i++)
		{
			array[i].sceneObject.transform.translation.x += offset_x;
			array[i].sceneObject.transform.translation.y += offset_y;
		}
	}

	*arrangedLayer = VuoLayer_makeGroup(children, VuoTransform2d_make(position, 0, VuoPoint2d_make(1,1)));
	*arrangedLayerEvent = true;

	VuoRelease(children);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
}
