/**
 * @file
 * vuo.layer.combine.center node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Combine Layers (Center)",
					  "keywords" : [ "group", "join", "together", "merge" ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions" : [ ]
					  }
				  });

#define VuoPoint3d_zero (VuoPoint3d) {0,0,0}
#define VuoPoint3d_one (VuoPoint3d) {1,1,1}
#define DEG_2_RAD 0.0174532925

void nodeEvent
(
		VuoInputData(VuoAnchor, {"default": {"horizontalAlignment":"center", "verticalAlignment":"center"}}) anchor,
		VuoInputData(VuoPoint2d, {"name":"Position", "default":{"x":0,"y":0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":360, "suggestedStep":15}) rotation,
		VuoInputData(VuoPoint2d, {"default":{"x":1,"y":1}, "suggestedMin":{"x":0,"y":0}, "suggestedMax":{"x":2,"y":2}, "suggestedStep":{"x":0.1,"y":0.1}}) scale,
		VuoInputData(VuoList_VuoLayer) layers,
		VuoOutputData(VuoLayer) layer
)
{

	*layer = VuoLayer_makeGroup(layers, VuoTransform2d_makeIdentity() );

	VuoBox bounds = VuoSceneObject_bounds( (*layer).sceneObject );

	VuoTransform toCenterMatrix = VuoTransform_makeEuler(
		(VuoPoint3d) { -bounds.center.x, -bounds.center.y, 0. },
		VuoPoint3d_zero,
		VuoPoint3d_one );

	VuoTransform rotationAndScale = VuoTransform_makeEuler(
		(VuoPoint3d) { 0, 0, 0},
		(VuoPoint3d) { 0, 0, rotation * DEG_2_RAD},
		(VuoPoint3d) { scale.x, scale.y, 1}
		);

	float cenMatrix[16], rotateScaleMatrix[16];
	VuoTransform_getMatrix(toCenterMatrix, (float*)cenMatrix);
	VuoTransform_getMatrix(rotationAndScale, (float*)rotateScaleMatrix);

	float applied[16];
	VuoTransform_multiplyMatrices4x4(cenMatrix, rotateScaleMatrix, applied);

	layer->sceneObject.transform = VuoTransform_makeFromMatrix4x4(applied);
	layer->sceneObject.transform.translation = VuoPoint3d_add(layer->sceneObject.transform.translation, (VuoPoint3d) { center.x, center.y, 0 });

	*layer = VuoLayer_setAnchor(*layer, anchor, -1, -1, -1);
}
