/**
 * @file
 * VuoImageConvolve interface.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "VuoImage.h"
#include "VuoThresholdType.h"
#include "VuoDiode.h"

typedef void *VuoImageConvolve; ///< State data for the image Convolverer.

VuoImageConvolve VuoImageConvolve_make(void);
VuoImage VuoImageConvolve_convolve(VuoImageConvolve convolve, VuoImage image, VuoImage convolutionMatrix, VuoThresholdType channels, double intensity, double threshold, VuoDiode range);

/// A function that can be passed to @ref VuoImageConvolve_generateMatrix.
typedef double (*VuoImageConvolveFunction)(double x, double y, double param);

double VuoImageConvolve_laplacianOfGaussian(double x, double y, double radius);
unsigned int VuoImageConvolve_laplacianOfGaussianWidth(double sigma);

VuoImage VuoImageConvolve_generateMatrix(VuoImageConvolveFunction f, unsigned int width, bool removeDCOffset, double param);

#ifdef __cplusplus
}
#endif
