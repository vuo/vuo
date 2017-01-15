/**
 * @file
 * vuo.image.sample.color node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLMacro.h>
#include "VuoShader.h"

VuoModuleMetadata({
					  "title" : "Sample Color from Image",
					  "keywords" : [ "coordinate", "eyedrop", "sample", "grab" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "ShowColorFromImage.vuo" ]
					  }
				 });

static inline int clampi(int value, int min, int max)
{
	return value < min ? min : (value > max ? max : value);
}

static const float UINT_TO_FLOAT_COLOR = .0039215686;

static void colorAtCoordinate(const unsigned char* pixels,
						const unsigned int width,
						const unsigned int height,
						const unsigned int x,
						const unsigned int y,
						float* r,
						float* g,
						float* b,
						float* a)
{
	const unsigned char* position = &pixels[((4 * width * y)) + (x * 4)];

	*r = ((unsigned int)position[2]) * UINT_TO_FLOAT_COLOR;
	*g = ((unsigned int)position[1]) * UINT_TO_FLOAT_COLOR;
	*b = ((unsigned int)position[0]) * UINT_TO_FLOAT_COLOR;
	*a = ((unsigned int)position[3]) * UINT_TO_FLOAT_COLOR;
}

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {	"default":{"x":0, "y":0},
									"suggestedMin":{"x":-1, "y":-1},
									"suggestedMax":{"x":1, "y":1}}) center,
		VuoInputData(VuoReal, {"default":0, "suggestedMin":0, "suggestedMax":2, "suggestedStep":0.01}) width,
		VuoOutputData(VuoColor) color
)
{
	if (!image)
		return;

	const unsigned char *pixelData = VuoImage_getBuffer(image, GL_BGRA);
	if (!pixelData)
		return;

	unsigned int w = image->pixelsWide;
	unsigned int h = image->pixelsHigh;
	float aspect = h/(float)w;
	unsigned int pixelRadius = clampi(((fmax(0, fmin(2, width))/2.) * (w/2)), 0, (w/2));	// clamp radius from 1px to image width / 2
	float 	r, g, b, a, rf = 0.f, gf = 0.f, bf = 0.f, af = 0.f;

	VuoPoint2d pixelCoord = VuoPoint2d_clamp( VuoShader_samplerCoordinatesFromVuoCoordinates(center, image), 0., 1. );

	pixelCoord.x *= w;
	pixelCoord.y *= h;

	unsigned int sampleCount = 0;

	for(int y = clampi(pixelCoord.y-pixelRadius, 0, h-1); y <= clampi(pixelCoord.y+pixelRadius, 0, h-1); y++)
	{
		for(int x = clampi(pixelCoord.x-pixelRadius, 0, w-1); x <= clampi(pixelCoord.x+pixelRadius, 0, w-1); x++)
		{
			sampleCount++;

			colorAtCoordinate(pixelData, w, h, x, y, &r, &g, &b, &a);

			rf += r;
			gf += g;
			bf += b;
			af += a;
		}
	}

	*color = VuoColor_makeWithRGBA(	rf/(float)sampleCount,
									gf/(float)sampleCount,
									bf/(float)sampleCount,
									af/(float)sampleCount);
}
