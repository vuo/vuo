/**
 * @file
 * vuo.noise.gradient node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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

static void VuoReal_getGradientRange(VuoReal *start, VuoReal *end)
{
	*start = -1;
	*end = 1;
}

static void VuoPoint2d_getGradientRange(VuoPoint2d *start, VuoPoint2d *end)
{
	*start = (VuoPoint2d){-1,-1};
	*end = (VuoPoint2d){1,1};
}

static void VuoPoint3d_getGradientRange(VuoPoint3d *start, VuoPoint3d *end)
{
	*start = (VuoPoint3d){-1,-1,-1};
	*end = (VuoPoint3d){1,1,1};
}
static void VuoPoint4d_getGradientRange(VuoPoint4d *start, VuoPoint4d *end)
{
	*start = (VuoPoint4d){-1,-1,-1,-1};
	*end = (VuoPoint4d){1,1,1,1};
}

void nodeEvent
(
	VuoInputData(VuoGenericType1, {"default":{"x":0,"y":0,"z":0,"w":0}}) position,
	VuoInputData(VuoGradientNoise, {"default":"perlin"}) gradientNoise,
	VuoInputData(VuoGenericType2, {"defaults":{"VuoReal":-1., "VuoPoint2d":{"x":-1.,"y":-1.}, "VuoPoint3d":{"x":-1.,"y":-1.,"z":-1.}, "VuoPoint4d":{"x":-1.,"y":-1.,"z":-1.,"w":-1.}}}) scaledStart,
	VuoInputData(VuoGenericType2, {"defaults":{"VuoReal":1., "VuoPoint2d":{"x":1.,"y":1.}, "VuoPoint3d":{"x":1.,"y":1.,"z":1.}, "VuoPoint4d":{"x":1.,"y":1.,"z":1.,"w":1.}}}) scaledEnd,
	VuoOutputData(VuoGenericType2) value
)
{
	if (gradientNoise == VuoGradientNoise_Perlin)
		*value = VuoGradientNoise_perlin_VuoGenericType1_VuoGenericType2(position);
	else if (gradientNoise == VuoGradientNoise_Simplex)
		*value = VuoGradientNoise_simplex_VuoGenericType1_VuoGenericType2(position);

	VuoGenericType2 start, end;
	VuoGenericType2_getGradientRange(&start, &end);
	VuoGenericType2 range = VuoGenericType2_subtract(end, start);
	range = VuoGenericType2_makeNonzero(range);
	VuoGenericType2 scaledRange = VuoGenericType2_subtract(scaledEnd, scaledStart);
	*value = VuoGenericType2_add( VuoGenericType2_divide( VuoGenericType2_scale( VuoGenericType2_subtract(*value, start), scaledRange), range), scaledStart);
}
