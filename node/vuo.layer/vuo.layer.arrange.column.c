/**
 * @file
 * vuo.layer.arrange.column node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"
#include "VuoList_VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Arrange Layers in Column",
					 "keywords" : [ "line", "place", "layout", "vertical", "row" ],
					 "version" : "1.1.0",
					 "node": {
						 "exampleCompositions" : [ "ShowArrangedLayers.vuo", "ShowArrangedTextLayers.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	uint64_t id;
	VuoRenderedLayers renderedLayers;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData* instance = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);
	instance->id = VuoSceneObject_getNextId();
	instance->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(instance->renderedLayers);
	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoRenderedLayers, {"name":"Window"}) renderedLayers,
		VuoInputEvent({"eventBlocking":"door","data":"renderedLayers"}) renderedLayersEvent,
		VuoInputData(VuoHorizontalAlignment, {"default":"center"}) horizontalAlignment,
		VuoInputData(VuoAnchor, {"default": {"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
		VuoInputData(VuoPoint2d, {"default":{"x":0, "y":0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) position,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":0.25, "suggestedStep":0.001}) spacing,
		VuoOutputData(VuoLayer) arrangedLayer
)
{
	bool renderingDimensionsChanged;
	VuoRenderedLayers_update((*instance)->renderedLayers, renderedLayers, &renderingDimensionsChanged);

	unsigned long viewportWidth = 0;
	unsigned long viewportHeight = 0;
	float backingScaleFactor = 1;
	// If `renderedLayers` has no dimensions, the above values are left unmodified.
	VuoIgnoreResult(VuoRenderedLayers_getRenderingDimensions((*instance)->renderedLayers, &viewportWidth, &viewportHeight, &backingScaleFactor));

	unsigned int count = VuoListGetCount_VuoLayer(layers);

	VuoList_VuoLayer children = VuoListCreate_VuoLayer();
	VuoRetain(children);

	VuoPoint3d currentPosition = VuoPoint3d_make(0,0,0);
	VuoReal size_x = 0;

	for(int i = 1; i <= count; i++)
	{
		VuoLayer child = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)VuoListGetValue_VuoLayer(layers, i));
		VuoRectangle bounds = VuoLayer_getBoundingRectangle(child, viewportWidth, viewportHeight, backingScaleFactor);

		size_x = fmax(size_x, bounds.size.x);

		VuoPoint3d translation = VuoSceneObject_getTranslation((VuoSceneObject)child);
		translation.x += currentPosition.x;
		translation.y += currentPosition.y - ((bounds.size.y * .5) + bounds.center.y);

		if(horizontalAlignment == VuoHorizontalAlignment_Right)
			translation.x += -bounds.center.x - (bounds.size.x * .5);
		else if(horizontalAlignment == VuoHorizontalAlignment_Center)
			translation.x += -bounds.center.x;
		else
			translation.x += -bounds.center.x + (bounds.size.x * .5);

		// translation now set to where each layer is aligned exactly in the row.  applying the original translation allows for tweaks
		translation.x += VuoSceneObject_getTranslation((VuoSceneObject)child).x;
		translation.y += VuoSceneObject_getTranslation((VuoSceneObject)child).y;

		VuoSceneObject_setTranslation((VuoSceneObject)child, translation);

		currentPosition.y -= bounds.size.y + spacing;
		VuoListAppendValue_VuoLayer(children, child);
	}

	{
		VuoReal center_x = 	horizontalAlignment == VuoHorizontalAlignment_Center ? 0 :
							horizontalAlignment == VuoHorizontalAlignment_Left ? size_x * .5 :
							size_x * -.5;

		VuoReal offset_x =  VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Left ? (-center_x + (size_x * .5)) :
							VuoAnchor_getHorizontal(anchor) == VuoHorizontalAlignment_Center ? -center_x :
							(-center_x - (size_x * .5));

		VuoReal height = VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Top ? 0 : fabs(currentPosition.y) - spacing;
		VuoReal offset_y = VuoAnchor_getVertical(anchor) == VuoVerticalAlignment_Center ? height * .5 : height;

		unsigned long childCount = VuoListGetCount_VuoLayer(children);
		VuoLayer* array = VuoListGetData_VuoLayer(children);

		for (int i = 0; i < childCount; i++)
			VuoSceneObject_translate((VuoSceneObject)array[i], (VuoPoint3d){offset_x, offset_y, 0});
	}

	*arrangedLayer = VuoLayer_makeGroup(children, VuoTransform2d_make(position, 0, VuoPoint2d_make(1,1)));
	VuoLayer_setId(*arrangedLayer, (*instance)->id);

	VuoRelease(children);
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) instance
)
{
	VuoRenderedLayers_release((*instance)->renderedLayers);
}
