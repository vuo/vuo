/**
 * @file
 * vuo.scene.make.light.ambient node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Make Ambient Light",
					  "keywords" : [ "draw", "opengl", "scenegraph", "graphics", "nondirectional", "background", "lighting" ],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "CompareLights.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) color,
		VuoInputData(VuoReal, {"default":0.1, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) brightness,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_makeAmbientLight(color, brightness);
}
