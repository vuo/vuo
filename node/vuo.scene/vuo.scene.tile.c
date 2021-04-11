/**
 * @file
 * vuo.scene.tile node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Tile 3D Object",
					  "keywords" : [
						  "copy", "filter",
						  "grid", "volume", "wrap", "infinite",
						  "stars", "starfield",
					  ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "TileStarfield.vuo" ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0},
							  "suggestedMin":{"x":-2.0,"y":-2.0,"z":-2.0},
							  "suggestedMax":{"x":2.0,"y":2.0,"z":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
	VuoInputData(VuoPoint3d, {"default":{"x":1.0,"y":1.0,"z":1.0},
							  "suggestedMin":{"x":0.1,"y":0.1,"z":0.1},
							  "suggestedMax":{"x":2.0,"y":2.0,"z":2.0},
							  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) spacing,
	VuoInputData(VuoPoint3d, {"default":{"x":2.0,"y":2.0,"z":0.0},
							  "suggestedMin":{"x":0.0,"y":0.0,"z":0.0},
							  "suggestedMax":{"x":10.0,"y":10.0,"z":10.0},
							  "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) fieldSize,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddColumns,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddRows,
//	VuoInputData(VuoBoolean, {"default":false}) reflectOddSlices,
//	VuoInputData(VuoBoolean, {"default":false}) fadeBoundaryCopies,
	VuoOutputData(VuoSceneObject) tiledObject
)
{
	*tiledObject = VuoSceneObject_makeGroup(VuoListCreate_VuoSceneObject(), VuoTransform_makeIdentity());

	VuoPoint3d clampedFieldSize = VuoPoint3d_makeNonzero(fieldSize);
	VuoPoint3d clampedSpacing = (VuoPoint3d){MAX(0.01,spacing.x), MAX(0.01,spacing.y), MAX(0.01,spacing.z)};
	VuoList_VuoSceneObject childObjects = VuoSceneObject_getChildObjects(*tiledObject);

	for (double z = remainder(center.z, clampedSpacing.z) - clampedFieldSize.z/2;  z < (clampedFieldSize.z + clampedSpacing.z) / 2.;  z += clampedSpacing.z)
	for (double y = remainder(center.y, clampedSpacing.y) - clampedFieldSize.y/2;  y < (clampedFieldSize.y + clampedSpacing.y) / 2.;  y += clampedSpacing.y)
	for (double x = remainder(center.x, clampedSpacing.x) - clampedFieldSize.x/2;  x < (clampedFieldSize.x + clampedSpacing.x) / 2.;  x += clampedSpacing.x)
	{
		VuoTransform transform = VuoTransform_makeEuler((VuoPoint3d){x,y,z},
														(VuoPoint3d){0,0,0},
														(VuoPoint3d){1,1,1});

		VuoSceneObject so = VuoSceneObject_copy(object);
		VuoSceneObject_transform(so, transform);
		VuoListAppendValue_VuoSceneObject(childObjects, so);
	}
}
