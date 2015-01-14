/**
 * @file
 * VuoGradientNoiseCommon interface.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoReal VuoGradientNoise_perlin1D(VuoReal x);
VuoReal VuoGradientNoise_perlin2D(VuoReal x, VuoReal y);
VuoReal VuoGradientNoise_perlin3D(VuoReal x, VuoReal y, VuoReal z);
VuoReal VuoGradientNoise_perlin4D(VuoReal x, VuoReal y, VuoReal z, VuoReal w);

VuoReal VuoGradientNoise_simplex1D(VuoReal x);
VuoReal VuoGradientNoise_simplex2D(VuoReal x, VuoReal y);
VuoReal VuoGradientNoise_simplex3D(VuoReal x, VuoReal y, VuoReal z);
VuoReal VuoGradientNoise_simplex4D(VuoReal x, VuoReal y, VuoReal z, VuoReal w);
