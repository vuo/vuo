/**
 * @file
 * vuo.noise.gradient node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGradientNoise.h"
#include "VuoGradientNoiseCommon.h"

VuoModuleMetadata({
					  "title" : "Make Gradient Noise",
					  "keywords" : [ "perlin", "simplex", "random", "pseudo", "natural" ],
					  "version" : "2.0.0",
					  "dependencies" : [ "VuoGradientNoiseCommon" ],
					  "genericTypes" : {
						  "VuoGenericType1" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  },
						  "VuoGenericType2" : {
							  "compatibleTypes" : [ "VuoReal", "VuoPoint2d", "VuoPoint3d", "VuoPoint4d" ]
						  }
					  },
					  "node": {
						  "exampleCompositions" : [ "WanderImage.vuo" ]
					  }
				  });

void nodeEvent
(
	VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) position,
	VuoInputData(VuoGradientNoise, {"default":"perlin"}) gradientNoise,
	VuoOutputData(VuoGenericType2) value
)
{
	if (gradientNoise == VuoGradientNoise_Perlin)
		*value = VuoGradientNoise_perlin_VuoGenericType1_VuoGenericType2(position);
	else if (gradientNoise == VuoGradientNoise_Simplex)
		*value = VuoGradientNoise_simplex_VuoGenericType1_VuoGenericType2(position);
}
