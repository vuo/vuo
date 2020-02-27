/**
 * @file
 * VuoImageMapColors implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include <OpenGL/CGLMacro.h>

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageMapColors",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ]
				 });
#endif
/// @}

/**
 * Returns a color-mapped copy of @a image.  (Or, if @a colors is empty, returns @a image.)
 */
VuoImage VuoImage_mapColors(VuoImage image, VuoList_VuoColor colors, VuoReal filterOpacity)
{
	if (!image)
		return NULL;

	if (!colors)
		return image;

	static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,

		uniform float gradientCount;
		uniform sampler2D image;		// the unfiltered image
		uniform sampler2D gradientStrip;// the gradient strip to map against
		uniform float amount;			// the amount to mix gradient and image
		varying vec2 fragmentTextureCoordinate;

		// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
		float brightness(vec3 col)
		{
			// Digital CCIR601 (gives more weight to the green and red components):
			return 0.299*col.r + 0.587*col.g + 0.114*col.b;
		}

		void main(void)
		{
			vec4 orig = texture2D(image, fragmentTextureCoordinate);

			float lum = brightness(orig.rgb/orig.a);

			float gradientWidth = (1./gradientCount)/2.;
			lum = lum * (1-gradientWidth*2) + gradientWidth;

			vec4 color = texture2D(gradientStrip, vec2(clamp(lum, gradientWidth, 1-gradientWidth), .5));

			vec4 mixed = mix(orig, color, amount);
			mixed *= orig.a;

			gl_FragColor = mixed;
		}
	);

	int len = VuoListGetCount_VuoColor(colors);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(unsigned char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValue_VuoColor(colors, i);
		// VuoColor is non-premultiplied, but VuoImage_makeFromBuffer() expects premultiplied colors, so premultiply them.
		pixels[n++] = (unsigned int)(col.a*col.b*255);
		pixels[n++] = (unsigned int)(col.a*col.g*255);
		pixels[n++] = (unsigned int)(col.a*col.r*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_BGRA, len, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	VuoShader shader = VuoShader_make("Map Image Colors Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(shader);

	// Set `image` before `gradientStrip`, so that `image`'s scaleFactor is used for the output image.
	VuoShader_setUniform_VuoImage(shader, "image", image);
	VuoShader_setUniform_VuoImage(shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoReal (shader, "gradientCount", (float)len);
	VuoShader_setUniform_VuoReal (shader, "amount", filterOpacity);

	// Render.
	VuoImage mappedImage = VuoImageRenderer_render(shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoRelease(shader);

	return mappedImage;
}
