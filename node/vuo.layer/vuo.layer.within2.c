/**
 * @file
 * vuo.layer.within2 node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"
#include "VuoRenderedLayers.h"

VuoModuleMetadata({
	"title" : "Is Point within Layer",
	"keywords" : [ "hit test", "contains", "bounds" ],
	"version" : "2.0.0",
	"node": {
		"exampleCompositions" : [ "IsMouseWithinLayer.vuo" ]
	}
});

struct nodeInstanceData
{
	VuoRenderedLayers renderedLayers;
};

struct nodeInstanceData * nodeInstanceInit()
{
	struct nodeInstanceData *context = (struct nodeInstanceData *)calloc(1,sizeof(struct nodeInstanceData));
	VuoRegister(context, free);
	context->renderedLayers = VuoRenderedLayers_makeEmpty();
	VuoRenderedLayers_retain(context->renderedLayers);
	return context;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) context,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}}) point,
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoRenderedLayers) window,
	VuoInputEvent({"data":"window","eventBlocking":"door"}) windowEvent,

	VuoOutputData(VuoBoolean) withinLayer
)
{
	if (windowEvent)
	{
		bool renderingDimensionsChanged;
		VuoRenderedLayers_update((*context)->renderedLayers, window, &renderingDimensionsChanged);
	}

	*withinLayer = VuoRenderedLayers_isPointInLayerId((*context)->renderedLayers, VuoLayer_getId(layer), point);
}

void nodeInstanceFini
(
	VuoInstanceData(struct nodeInstanceData *) context
)
{
	VuoRenderedLayers_release((*context)->renderedLayers);
}
