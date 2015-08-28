/**
 * @file
 * vuo.layer.arrange.grid node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Arrange Layers in Grid",
					 "keywords" : [ "align", "place", "position", "lattice", "matrix" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ "ChangeGridSpacing.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoLayer) layers,
		VuoInputData(VuoBoolean, {"default":true}) scaleToFit,
		VuoInputData(VuoPoint2d, {"default":[{"x":0,"y":0}], "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) width,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1}) columns,
		VuoInputData(VuoReal, {"default":2.0, "suggestedMin":0.0, "suggestedStep":0.1}) height,
		VuoInputData(VuoInteger, {"default":2, "suggestedMin":1}) rows,
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

			VuoLayer layer = VuoListGetValueAtIndex_VuoLayer(layers, currentLayer);

			if (scaleToFit)
			{
				VuoSceneObject_normalize(&layer.sceneObject);
				layer.sceneObject.transform.scale = VuoPoint3d_multiply(layer.sceneObject.transform.scale, fmin(width/columns, height/rows));
			}

			layer.sceneObject.transform.translation = VuoPoint3d_add(layer.sceneObject.transform.translation, VuoPoint3d_make(xPosition,yPosition,0));

			VuoListAppendValue_VuoLayer(childLayers, layer);

			++currentLayer;
			if (currentLayer > layerCount)
				goto done;
		}
	}

done:
	*griddedLayer = VuoLayer_makeGroup(childLayers, VuoTransform2d_makeIdentity());
	VuoRelease(childLayers);
}
