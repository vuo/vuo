/**
 * @file
 * vuo.layer.arrange.grid node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Arrange Layers in Grid",
					 "keywords" : [ "alignment", "line", "place", "layout", "lattice", "matrix" ],
					 "version" : "1.1.0",
					 "node": {
						 "exampleCompositions" : [ "ChangeGridSpacing.vuo" ]
					 }
				 });

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
		VuoInstanceData(uint64_t) id,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoBoolean, {"name":"Scale to Fit", "default":true}) scaleToFit,
		VuoInputData(VuoAnchor, {"default": {"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1, "suggestedMax":8}) columns,
		VuoInputData(VuoReal, {"default":1.5, "suggestedMin":0.0, "suggestedMax":2.0, "suggestedStep":0.1}) height,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1, "suggestedMax":8}) rows,
		VuoOutputData(VuoLayer) griddedLayer
)
{
	VuoList_VuoLayer childLayers = VuoListCreate_VuoLayer();
	VuoRetain(childLayers);

	unsigned long layerCount = VuoListGetCount_VuoLayer(layers);
	unsigned long currentLayer = 1;

	for (unsigned int y = 0; y < rows; ++y)
	{
		VuoReal yPosition = center.y + height/2 - (height / rows) * (y + .5);

		for (unsigned int x = 0; x < columns; ++x)
		{
			VuoReal xPosition = center.x - width/2 + (width / columns) * (x + .5);

			VuoLayer layer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)VuoListGetValue_VuoLayer(layers, currentLayer));

			if (scaleToFit)
			{
				VuoRectangle bounds = VuoLayer_getBoundingRectangle(layer, -1, -1, 1);

				VuoReal gridCellWidth = width / columns;
				VuoReal gridCellHeight = height / rows;
				VuoReal gridCellAspect = gridCellWidth / gridCellHeight;
				VuoReal layerWidth = bounds.size.x;
				VuoReal layerHeight = bounds.size.y;
				VuoReal layerAspect = layerWidth / layerHeight;
				VuoReal scaleFactor = (layerAspect > gridCellAspect ? gridCellWidth / layerWidth : gridCellHeight / layerHeight);

				VuoSceneObject_scale((VuoSceneObject)layer, (VuoPoint3d){scaleFactor, scaleFactor, scaleFactor});
			}

			VuoSceneObject_translate((VuoSceneObject)layer, (VuoPoint3d){xPosition, yPosition, 0});

			VuoListAppendValue_VuoLayer(childLayers, layer);

			++currentLayer;
			if (currentLayer > layerCount)
				goto done;
		}
	}

done:
	*griddedLayer = VuoLayer_setAnchor(VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity()),
									   anchor, -1, -1, -1);
	VuoLayer_setId(*griddedLayer, *id);
	VuoRelease(childLayers);
}
