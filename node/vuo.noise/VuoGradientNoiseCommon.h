/**
 * @file
 * VuoGradientNoiseCommon interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "VuoReal.h"
#include "VuoPoint2d.h"
#include "VuoPoint3d.h"
#include "VuoPoint4d.h"

VuoReal VuoGradientNoise_perlin_VuoReal_VuoReal(VuoReal x);
VuoPoint2d VuoGradientNoise_perlin_VuoReal_VuoPoint2d(VuoReal x);
VuoPoint3d VuoGradientNoise_perlin_VuoReal_VuoPoint3d(VuoReal x);
VuoPoint4d VuoGradientNoise_perlin_VuoReal_VuoPoint4d(VuoReal x);

VuoReal VuoGradientNoise_perlin_VuoPoint2d_VuoReal(VuoPoint2d point);
VuoPoint2d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint2d(VuoPoint2d point);
VuoPoint3d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint3d(VuoPoint2d point);
VuoPoint4d VuoGradientNoise_perlin_VuoPoint2d_VuoPoint4d(VuoPoint2d point);

VuoReal VuoGradientNoise_perlin_VuoPoint3d_VuoReal(VuoPoint3d point);
VuoPoint2d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint2d(VuoPoint3d point);
VuoPoint3d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint3d(VuoPoint3d point);
VuoPoint4d VuoGradientNoise_perlin_VuoPoint3d_VuoPoint4d(VuoPoint3d point);

VuoReal VuoGradientNoise_perlin_VuoPoint4d_VuoReal(VuoPoint4d point);
VuoPoint2d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint2d(VuoPoint4d point);
VuoPoint3d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint3d(VuoPoint4d point);
VuoPoint4d VuoGradientNoise_perlin_VuoPoint4d_VuoPoint4d(VuoPoint4d point);

VuoReal VuoGradientNoise_simplex_VuoReal_VuoReal(VuoReal x);
VuoPoint2d VuoGradientNoise_simplex_VuoReal_VuoPoint2d(VuoReal x);
VuoPoint3d VuoGradientNoise_simplex_VuoReal_VuoPoint3d(VuoReal x);
VuoPoint4d VuoGradientNoise_simplex_VuoReal_VuoPoint4d(VuoReal x);

VuoReal VuoGradientNoise_simplex_VuoPoint2d_VuoReal(VuoPoint2d point);
VuoPoint2d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint2d(VuoPoint2d point);
VuoPoint3d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint3d(VuoPoint2d point);
VuoPoint4d VuoGradientNoise_simplex_VuoPoint2d_VuoPoint4d(VuoPoint2d point);

VuoReal VuoGradientNoise_simplex_VuoPoint3d_VuoReal(VuoPoint3d point);
VuoPoint2d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint2d(VuoPoint3d point);
VuoPoint3d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint3d(VuoPoint3d point);
VuoPoint4d VuoGradientNoise_simplex_VuoPoint3d_VuoPoint4d(VuoPoint3d point);

VuoReal VuoGradientNoise_simplex_VuoPoint4d_VuoReal(VuoPoint4d point);
VuoPoint2d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint2d(VuoPoint4d point);
VuoPoint3d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint3d(VuoPoint4d point);
VuoPoint4d VuoGradientNoise_simplex_VuoPoint4d_VuoPoint4d(VuoPoint4d point);
