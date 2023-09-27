/**
 * @file
 * vuo.layer.blendMode node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoLayer.h"

VuoModuleMetadata({
					  "title" : "Change Layer Blending",
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
						  "exampleCompositions" : [ "ChangeLayerBlendMode.vuo" ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoLayer) layer,
	VuoInputData(VuoBlendMode, {"default":"linear-dodge", "restrictToOpenGlBlendModes":true}) blendMode,
	VuoOutputData(VuoLayer) blendedLayer
)
{
	*blendedLayer = (VuoLayer)VuoSceneObject_copy((VuoSceneObject)layer);
	VuoSceneObject_setBlendMode((VuoSceneObject)*blendedLayer, blendMode);
}
