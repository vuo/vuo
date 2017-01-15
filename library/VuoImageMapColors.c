/**
 * @file
 * VuoImageMapColors implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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

	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer imageRenderer = VuoImageRenderer_make(glContext);
	VuoRetain(imageRenderer);

	static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,

		uniform float gradientCount;
		uniform sampler2D image;		// the unfiltered image
		uniform sampler2D gradientStrip;// the gradient strip to map against
		uniform float amount;			// the amount to mix gradient and image
		varying vec4 fragmentTextureCoordinate;

		// https://stackoverflow.com/questions/596216/formula-to-determine-brightness-of-rgb-color
		float brightness(vec3 col)
		{
			// Digital CCIR601 (gives more weight to the green and red components):
			return 0.299*col.r + 0.587*col.g + 0.114*col.b;
		}

		void main(void)
		{
			vec4 orig = texture2D(image, fragmentTextureCoordinate.xy);

			float lum = brightness(orig.rgb);

			float gradientWidth = (1./gradientCount)/2.;
			lum = lum * (1-gradientWidth*2) + gradientWidth;

			vec4 color = texture2D(gradientStrip, vec2(clamp(lum, gradientWidth, 1-gradientWidth), .5));

			gl_FragColor = mix(orig, color, amount) * orig.a;
		}
	);

	int len = VuoListGetCount_VuoColor(colors);

	unsigned char* pixels = (unsigned char*)malloc(sizeof(unsigned char)*len*4);
	int n = 0;
	for(int i = 1; i <= len; i++)
	{
		VuoColor col = VuoListGetValue_VuoColor(colors, i);
		pixels[n++] = (unsigned int)(col.b*255);
		pixels[n++] = (unsigned int)(col.g*255);
		pixels[n++] = (unsigned int)(col.r*255);
		pixels[n++] = (unsigned int)(col.a*255);
	}

	VuoImage gradientStrip = VuoImage_makeFromBuffer(pixels, GL_BGRA, len, 1, VuoImageColorDepth_8, ^(void *buffer){ free(buffer); });

	VuoShader shader = VuoShader_make("Map Image Colors Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(shader);

	VuoShader_setUniform_VuoImage(shader, "gradientStrip", gradientStrip);
	VuoShader_setUniform_VuoImage(shader, "image", image);
	VuoShader_setUniform_VuoReal (shader, "gradientCount", (float)len);
	VuoShader_setUniform_VuoReal (shader, "amount", filterOpacity);

	// Render.
	VuoImage mappedImage = VuoImageRenderer_draw(imageRenderer, shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));

	VuoRelease(shader);

	VuoRelease(imageRenderer);
	VuoGlContext_disuse(glContext);

	return mappedImage;
}
