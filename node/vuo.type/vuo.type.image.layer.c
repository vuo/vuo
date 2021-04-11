/**
 * @file
 * vuo.type.image.layer node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "../vuo.layer/VuoLayer.h"

VuoModuleMetadata({
					 "title" : "Convert Image to Layer",
					 "keywords" : [ "billboard", "sprite", "scaled",
						 "stretch", "fill", "shrink", "blow up", "enlarge", "magnify", "render" ],
					 "version" : "1.0.0",
					 "node": {
						  "exampleCompositions" : [ "vuo-example://vuo.audio/ShowStabilizedAudioWaveform.vuo",
									    "vuo-example://vuo.image/CompareNoiseTypes.vuo",
									    "vuo-example://vuo.image/SimulateMotionBlur.vuo" ]
					 }
				 });

uint64_t nodeInstanceInit(void)
{
	return VuoSceneObject_getNextId();
}

void nodeInstanceEvent
(
		VuoInstanceData(uint64_t) id,
		VuoInputData(VuoImage) image,
		VuoOutputData(VuoLayer) layer
)
{
	*layer = VuoLayer_make(NULL, image, VuoPoint2d_make(0,0), 0, 2, VuoOrientation_Horizontal, 1);
	VuoLayer_setId(*layer, *id);
}
