/**
 * @file
 * VuoImageBlur implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageBlur",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ]
				 });
#endif
/// @}

/**
 * Returns a blurred copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * If @a expandBounds is true, the output image will be expanded to encompass the full bleed of the blur.
 * Otherwise, the output image will have the same dimensions as the soure image.
 */
VuoImage VuoImage_blur(VuoImage image, VuoReal radius, VuoBoolean expandBounds)
{
	if (!image)
		return NULL;

	if (radius < 0.0001)
		return image;

	VuoGlContext glContext = VuoGlContext_use();
	VuoImageRenderer imageRenderer = VuoImageRenderer_make(glContext);
	VuoRetain(imageRenderer);

	int inset = expandBounds ? ceil(radius) : 0;

	// @@@ Currently it looks like the blur actually blurs more than the specified radius..?
	inset += 2;

	int w = image->pixelsWide + inset*2;
	int h = image->pixelsHigh + inset*2;

	VuoImage img = image;

	// Multiple pass Gaussian blur implementation adapted from:
	// http://www.geeks3d.com/20100909/shader-library-gaussian-blur-post-processing-filter-in-glsl/
	static const char * verticalPassFragmentShader = VUOSHADER_GLSL_SOURCE(120,

		varying vec4 fragmentTextureCoordinate;

		uniform sampler2D texture;

		uniform float width; 	// render target width
		uniform float height; 	// render target height
		uniform float inset;

		uniform float offset[3];
		uniform float weight[3];

		void main(void)
		{
			vec2 uv = fragmentTextureCoordinate.xy;
			// Inset the source image relative to the output image to account for the optional bleed.
			uv.x = (uv.x - inset/width)  *  width/(width -inset*2.);
			uv.y = (uv.y - inset/height) * height/(height-inset*2.);

			vec4 tc = texture2D(texture, uv);
			tc *= weight[0];

			for (int i=1; i<3; i++)
			{
				tc += texture2D(texture, uv + vec2(0.0, offset[i])/height) * weight[i];
				tc += texture2D(texture, uv - vec2(0.0, offset[i])/height) * weight[i];
			}

			gl_FragColor = tc;
		}
	);

	static const char * horizontalPassFragmentShader = VUOSHADER_GLSL_SOURCE(120,

		varying vec4 fragmentTextureCoordinate;

		uniform sampler2D texture;

		uniform float width; 	// render target width

		uniform float offset[3];
		uniform float weight[3];

		void main(void)
		{
			vec2 uv = fragmentTextureCoordinate.xy;
			vec4 tc = texture2D(texture, uv);
			tc *= weight[0];

			for (int i=1; i<3; i++)
			{
				tc += texture2D(texture, uv + vec2(offset[i])/width, 0.0) * weight[i];
				tc += texture2D(texture, uv - vec2(offset[i])/width, 0.0) * weight[i];
			}

			gl_FragColor = tc;
		}
	);

	VuoShader verticalPassShader = VuoShader_make("Gaussian Blur Shader (Vertical)");
	VuoShader_addSource(verticalPassShader, VuoMesh_IndividualTriangles, NULL, NULL, verticalPassFragmentShader);
	VuoRetain(verticalPassShader);

	VuoShader horizontalPassShader = VuoShader_make("Gaussian Blur Shader (Horizontal)");
	VuoShader_addSource(horizontalPassShader, VuoMesh_IndividualTriangles, NULL, NULL, horizontalPassFragmentShader);
	VuoRetain(horizontalPassShader);

	VuoShader_setUniform_VuoReal(verticalPassShader,   "inset",  inset);
	VuoShader_setUniform_VuoReal(verticalPassShader,   "width",  w);
	VuoShader_setUniform_VuoReal(verticalPassShader,   "height", h);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "width",  w);

	VuoShader_setUniform_VuoReal(verticalPassShader, "offset[0]", 0);
	VuoShader_setUniform_VuoReal(verticalPassShader, "offset[1]", 1.3846153846);
	VuoShader_setUniform_VuoReal(verticalPassShader, "offset[2]", 3.2307692308);
	VuoShader_setUniform_VuoReal(verticalPassShader, "weight[0]", 0.2270270270);
	VuoShader_setUniform_VuoReal(verticalPassShader, "weight[1]", 0.3162162162);
	VuoShader_setUniform_VuoReal(verticalPassShader, "weight[2]", 0.0702702703);

	VuoShader_setUniform_VuoReal(horizontalPassShader, "offset[0]", 0);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "offset[1]", 1.3846153846);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "offset[2]", 3.2307692308);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "weight[0]", 0.2270270270);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "weight[1]", 0.3162162162);
	VuoShader_setUniform_VuoReal(horizontalPassShader, "weight[2]", 0.0702702703);

	for(int i = 0; i < radius; i++)
	{
		// apply vertical pass
		VuoShader_setUniform_VuoImage(verticalPassShader, "texture", img);

		VuoImage verticalPassImage = VuoImageRenderer_draw(imageRenderer, verticalPassShader, w, h, VuoImage_getColorDepth(img));

		// Only inset the first pass
		if (inset)
		{
			inset=0;
			VuoShader_setUniform_VuoReal(verticalPassShader, "inset", inset);
		}

		// apply horizontal pass
		VuoShader_setUniform_VuoImage(horizontalPassShader, "texture", verticalPassImage);

		// one pass complete, ready for another (or not)
		img = VuoImageRenderer_draw(imageRenderer, horizontalPassShader, w, h, VuoImage_getColorDepth(img));
	}

	VuoRelease(verticalPassShader);
	VuoRelease(horizontalPassShader);

	VuoRelease(imageRenderer);
	VuoGlContext_disuse(glContext);

	return img;
}
