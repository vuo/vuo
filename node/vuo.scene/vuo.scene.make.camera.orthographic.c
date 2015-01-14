/**
 * @file
 * vuo.scene.make.camera.orthographic node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Orthographic Camera",
					 "keywords" : [ "axonometric", "elevation", "isometric", "orthogonal", "parallel", "projection", "draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoText, {"default":"camera"}) name,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":1.0}}) position,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}}) rotation,
		VuoInputData(VuoReal, {"default":2.0,"suggestedMin":0.01}) width,
		VuoInputData(VuoReal, {"default":-10.0}) distanceMin,	// The orthographic camera can have zero or negative clip planes (unlike the perspective camera).
		VuoInputData(VuoReal, {"default":10.0}) distanceMax,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeOrthographicCamera(
				name,
				position,
				VuoPoint3d_multiply(rotation,M_PI/180.f),
				width,
				distanceMin,
				distanceMax
				);
}
