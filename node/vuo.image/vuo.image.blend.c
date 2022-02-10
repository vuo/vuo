/**
 * @file
 * vuo.image.blend node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoBlendMode.h"
#include "VuoImageBlend.h"

VuoModuleMetadata({
					 "title" : "Blend Images",
					 "keywords" : [ "combine", "mix", "fade", "merge", "layer", "composite", "channel",
						"normal", "add", "additive", "alpha", "opacity", "transparent", "transparency",
						"multiply", "darker", "linear burn", "color burn", "burn",
						"screen", "lighter", "linear dodge", "color dodge", "dodge",
						"overlay", "soft light", "hard light", "vivid light", "linear light", "pin light", "light", "hard mix",
						"difference", "exclusion", "subtract", "divide", "power",
						"hue", "saturation", "desaturate", "grayscale", "greyscale", "color", "luminosity", "filter" ],
					 "version" : "1.2.3",
					 "dependencies" : [
						 "VuoImageBlend"
					 ],
					 "node": {
						 "exampleCompositions" : [ "BlendImages.vuo", "SimulateFilmProjector.vuo" ]
					 }
				 });

struct nodeInstanceData
{
	VuoImageBlend blend;
	VuoInteger currentBlendMode;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->blend = NULL;
	instance->currentBlendMode = -1;	// set this to a negative value on initialization, forcing shader init on first event.

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) background,
		VuoInputData(VuoImage) foreground,
		VuoInputData(VuoBlendMode, {"default":"normal"}) blendMode,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":1,"suggestedStep":0.1}) foregroundOpacity,
		VuoInputData(VuoBoolean, {"default":true}) replaceOpacity,
		VuoOutputData(VuoImage, {"name":"Blended Image"}) blended
)
{
	if (blendMode != (*instance)->currentBlendMode)
	{
		VuoRelease((*instance)->blend);
		(*instance)->blend = VuoImageBlend_make(blendMode);
		VuoRetain((*instance)->blend);

		(*instance)->currentBlendMode = blendMode;
	}

	*blended = VuoImageBlend_blend((*instance)->blend, background, foreground, foregroundOpacity, replaceOpacity);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->blend);
}
