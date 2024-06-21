/**
 * @file
 * VuoErrorDiffusionDither interface.
 *
 * @copyright Copyright © 2012–2024 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "VuoMacOSSDKWorkaround.h"
#import <IOSurface/IOSurface.h>

/**
 * A state object for performing error-diffusion dithering.
 */
typedef void *VuoErrorDiffusionDither;

/**
 * Keep this in sync with the Metal MSL copy in VuoErrorDiffusionDither.m.
 */
typedef struct {
	int8_t xOffset;
	uint8_t yOffset;
	uint8_t weight;
} VuoDitherDiffusionStep;

/**
 * The max size of the statically-allocated buffer for sparse matrix values.
 *
 * Keep this in sync with the Metal MSL copy in VuoErrorDiffusionDither.m.
 */
#define VuoErrorDiffusionDither_maxSteps 12

/**
 * A sparse-matrix representation of how quantization error should be distributed to nearby pixels.
 *
 * Ensure that steps[] has a number of entries that's a multiple of 4 — pad with weight=0 if needed.
 *
 * Keep this in sync with the Metal MSL copy in VuoErrorDiffusionDither.m.
 */
typedef struct {
	VuoDitherDiffusionStep steps[VuoErrorDiffusionDither_maxSteps];
	uint8_t stepCount;
	uint8_t divisor;  // Can't simply sum `steps[].weight`, since e.g. Atkinson's divisor isn't equal to its sum.
} VuoDitherDiffusionMatrix;

VuoErrorDiffusionDither VuoErrorDiffusionDither_make(void);
IOSurfaceRef VuoErrorDiffusionDither_processImage(VuoErrorDiffusionDither edd, IOSurfaceRef image, VuoList_VuoColor palette, float colorDistanceThreshold, VuoDitherDiffusionMatrix diffusionMatrix, float ditheringIntensity);

#ifdef __cplusplus
}
#endif
