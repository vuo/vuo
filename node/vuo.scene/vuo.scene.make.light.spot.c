/**
 * @file
 * vuo.scene.make.light.spot node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Spotlight",
					  "keywords" : [ "draw", "opengl", "scenegraph", "graphics", "directional", "lighting", "point", "source" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "AimFlashlight.vuo", "CompareLights.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) color,
		VuoInputData(VuoReal, {"default":1.0, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
		VuoInputData(VuoPoint3d, {"default":{"x":0.,"y":0.,"z":1.}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) position,
		VuoInputData(VuoPoint2d, {"default":{"x":0.,"y":0.}, "suggestedMin":{"x":0.0,"y":0.0}, "suggestedMax":{"x":360.0,"y":360.0}, "suggestedStep":{"x":15.0,"y":15.0}}) rotation,
		VuoInputData(VuoReal, {"default":30., "suggestedMin":0., "suggestedMax":180., "suggestedStep":15.}) cone,
		VuoInputData(VuoReal, {"default":10., "suggestedMin":0., "suggestedStep":0.1}) range,
		VuoInputData(VuoReal, {"default":0.9, "suggestedMin":0., "suggestedMax":1., "suggestedStep":0.1}) sharpness,
		VuoOutputData(VuoSceneObject) object
)
{
	// For vector (1,0,0), Euler rotations about the x, y, and z axes respectively mean roll, yaw, and pitch.
	// Since spotlights are unaffected by roll, this node only takes 2 rotational values.
	VuoPoint3d rotation3d = VuoPoint3d_make(0, rotation.x+90., rotation.y);

	*object = VuoSceneObject_makeSpotlight(color, brightness, VuoTransform_makeEuler(position, VuoPoint3d_multiply(rotation3d, M_PI/180.), VuoPoint3d_make(1,1,1)), cone*M_PI/180., range, sharpness);
}
