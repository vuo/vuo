/**
 * @file
 * VuoImageRotate implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRotate.h"

#include "VuoLayer.h"
#include "VuoSceneRenderer.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
	"title": "VuoImageRotate",
	"dependencies": [
		"VuoLayer",
		"VuoSceneRenderer",
	]
});
#endif
/// @}

/**
 * Initializes image rotator state.
 *
 * It's OK to use the same state to rotate multiple images of different sizes and types.
 */
VuoImageRotate VuoImageRotate_make(void)
{
	return VuoSceneRenderer_make(1);
}

/**
 * Outputs the width and height of the minimal axis-aligned bounding box after rotating the image.
 */
static void VuoImageRotate_getTargetDimensions(int width, int height, float rotation_radians, int *resized_width, int *resized_height)
{
	VuoPoint2d corners[3] = {
		(VuoPoint2d){width, 0},
		(VuoPoint2d){width, height},
		(VuoPoint2d){0, height},
	};

	float x_min = 0., x_max = 0., y_min = 0., y_max = 0.;
	for (int i = 0; i < 3; ++i)
	{
		float ru = cos(rotation_radians) * corners[i].x + sin(rotation_radians) * corners[i].y;
		float rv = cos(rotation_radians) * corners[i].y - sin(rotation_radians) * corners[i].x;

		x_min = fmin(ru, x_min);
		y_min = fmin(rv, y_min);

		x_max = fmax(ru, x_max);
		y_max = fmax(rv, y_max);
	}

	*resized_width = x_max - x_min;
	*resized_height = y_max - y_min;
}

/**
 * Returns a new, rotated copy of the specified image.
 */
VuoImage VuoImageRotate_rotate(VuoImage image, VuoImageRotate rotator, VuoReal angleInDegrees, VuoBoolean expandBounds)
{
	VuoSceneRenderer sceneRenderer = rotator;

	if (!image)
		return NULL;

	int width = image->pixelsWide;
	int height = image->pixelsHigh;
	float layerWidth = 2.;

	if (expandBounds)
	{
		VuoImageRotate_getTargetDimensions(image->pixelsWide, image->pixelsHigh, angleInDegrees * M_PI / 180., &width, &height);
		layerWidth = fmax(image->pixelsWide / (float)width, image->pixelsHigh / (float)height) * 2;
	}

	VuoSceneObject rootSceneObject = (VuoSceneObject)VuoLayer_make(VuoText_make("Rotated Image"), image, (VuoPoint2d){0,0}, angleInDegrees, layerWidth, VuoOrientation_Horizontal, 1.);

	VuoSceneRenderer_setRootSceneObject(sceneRenderer, rootSceneObject);
	VuoSceneRenderer_regenerateProjectionMatrix(sceneRenderer, width, height);

	VuoImage rotatedImage;
	VuoSceneRenderer_renderToImage(sceneRenderer, &rotatedImage, VuoImage_getColorDepth(image), VuoMultisample_4, NULL, false);
	rotatedImage->scaleFactor = image->scaleFactor;
	return rotatedImage;
}
