/**
 * @file
 * vuo.noise.gradient.3d node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGradientNoise.h"
#include "VuoGradientNoiseCommon.h"

VuoModuleMetadata({
					 "title" : "Make Gradient Noise 3D",
					 "keywords" : [ "perlin", "simplex", "random", "pseudo", "natural" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGradientNoiseCommon"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
	VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) position,
	VuoInputData(VuoGradientNoise, {"default":"perlin"}) gradientNoise,
	VuoOutputData(VuoReal) value
)
{
	if (gradientNoise == VuoGradientNoise_Perlin)
		*value = VuoGradientNoise_perlin3D(position.x, position.y, position.z);
	else if (gradientNoise == VuoGradientNoise_Simplex)
		*value = VuoGradientNoise_simplex3D(position.x, position.y, position.z);
}
