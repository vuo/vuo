/**
 * @file
 * VuoGradientNoiseCommon interface.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoReal VuoGradientNoise_perlin_VuoReal(VuoReal x);
VuoReal VuoGradientNoise_perlin_VuoPoint2d(VuoPoint2d point);
VuoReal VuoGradientNoise_perlin_VuoPoint3d(VuoPoint3d point);
VuoReal VuoGradientNoise_perlin_VuoPoint4d(VuoPoint4d point);

VuoReal VuoGradientNoise_simplex_VuoReal(VuoReal x);
VuoReal VuoGradientNoise_simplex_VuoPoint2d(VuoPoint2d point);
VuoReal VuoGradientNoise_simplex_VuoPoint3d(VuoPoint3d point);
VuoReal VuoGradientNoise_simplex_VuoPoint4d(VuoPoint4d point);
