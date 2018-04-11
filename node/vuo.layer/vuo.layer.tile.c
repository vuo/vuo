/**
 * @file
 * vuo.layer.tile node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Tile Layer",
					  "keywords" : [
						  "copy", "filter",
						  "grid", "wrap", "infinite",
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0},
							  "suggestedMin":{"x":-2.0,"y":-2.0},
							  "suggestedMax":{"x":2.0,"y":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) center,
	VuoInputData(VuoPoint2d, {"default":{"x":1.0,"y":1.0},
							  "suggestedMin":{"x":0.1,"y":0.1},
							  "suggestedMax":{"x":2.0,"y":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) spacing,
	VuoInputData(VuoPoint2d, {"default":{"x":2.0,"y":2.0},
							  "suggestedMin":{"x":0.0,"y":0.0},
							  "suggestedMax":{"x":10.0,"y":10.0},
							  "suggestedStep":{"x":0.1,"y":0.1}}) fieldSize,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddColumns,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddRows,
//	VuoInputData(VuoBoolean, {"default":false}) fadeBoundaryCopies,
	VuoOutputData(VuoLayer) tiledLayer
)
{
	(*tiledLayer).sceneObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	VuoPoint2d clampedFieldSize = VuoPoint2d_makeNonzero(fieldSize);
	VuoPoint2d clampedSpacing = (VuoPoint2d){MAX(0.01,spacing.x), MAX(0.01,spacing.y)};

	for (double y = remainder(center.y, clampedSpacing.y) - clampedFieldSize.y/2;  y < (clampedFieldSize.y + clampedSpacing.y) / 2.;  y += clampedSpacing.y)
	for (double x = remainder(center.x, clampedSpacing.x) - clampedFieldSize.x/2;  x < (clampedFieldSize.x + clampedSpacing.x) / 2.;  x += clampedSpacing.x)
	{
		VuoTransform transform = VuoTransform_makeEuler((VuoPoint3d){x,y,0},
														(VuoPoint3d){0,0,0},
														(VuoPoint3d){1,1,1});

		VuoSceneObject so = layer.sceneObject;
		so.transform = VuoTransform_composite(layer.sceneObject.transform, transform);
		VuoListAppendValue_VuoSceneObject((*tiledLayer).sceneObject.childObjects, so);
	}
}
