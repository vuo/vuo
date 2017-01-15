/**
 * @file
 * vuo.scene.blendMode node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					  "title" : "Change 3D Object Blending",
					  "keywords" : [ "combine", "mix", "fade", "merge", "layer", "composite", "channel",
						 "normal", "add", "additive", "alpha", "opacity", "transparent", "transparency",
						 "multiply", "darker", "linear burn", "color burn", "burn",
						 "screen", "lighter", "linear dodge", "color dodge", "dodge",
						 "overlay", "soft light", "hard light", "vivid light", "linear light", "pin light", "light", "hard mix",
						 "difference", "exclusion", "subtract", "divide",
						 "hue", "saturation", "color", "luminosity",
						 "tint", "tone", "chroma" ],
					  "version" : "1.0.0",
					  "node" : {
						  "exampleCompositions" : [ "BlendSphereIntoScene.vuo" ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoBlendMode, {"default":"linear-dodge", "restrictToOpenGlBlendModes":true}) blendMode,
	VuoOutputData(VuoSceneObject) blendedObject
)
{
	*blendedObject = VuoSceneObject_copy(object);
	VuoSceneObject_setBlendMode(blendedObject, blendMode);
}
