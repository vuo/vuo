/**
 * @file
 * VuoImageBlur implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"
#include "VuoImageBlur.h"

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
 * State data for the image blurrer.
 */
typedef struct
{
	VuoShader verticalPassShader;
	VuoShader horizontalPassShader;
	VuoGlContext context;
	VuoImageRenderer imageRenderer;
} VuoImageBlur_internal;

/**
 * Frees image blender state data.
 */
void VuoImageBlur_free(void *blur)
{
	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;
	VuoRelease(bi->verticalPassShader);
	VuoRelease(bi->horizontalPassShader);
	VuoRelease(bi->imageRenderer);
	VuoGlContext_disuse(bi->context);
}

/**
 * Creates state data for the image blurrer.
 */
VuoImageBlur VuoImageBlur_make(void)
{
	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)malloc(sizeof(VuoImageBlur_internal));
	VuoRegister(bi, VuoImageBlur_free);


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

	bi->verticalPassShader = VuoShader_make("Gaussian Blur Shader (Vertical)");
	VuoShader_addSource(bi->verticalPassShader, VuoMesh_IndividualTriangles, NULL, NULL, verticalPassFragmentShader);
	VuoRetain(bi->verticalPassShader);

	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "offset[0]", 0);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "offset[1]", 1.3846153846);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "offset[2]", 3.2307692308);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "weight[0]", 0.2270270270);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "weight[1]", 0.3162162162);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader, "weight[2]", 0.0702702703);

	bi->horizontalPassShader = VuoShader_make("Gaussian Blur Shader (Horizontal)");
	VuoShader_addSource(bi->horizontalPassShader, VuoMesh_IndividualTriangles, NULL, NULL, horizontalPassFragmentShader);
	VuoRetain(bi->horizontalPassShader);

	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "offset[0]", 0);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "offset[1]", 1.3846153846);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "offset[2]", 3.2307692308);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "weight[0]", 0.2270270270);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "weight[1]", 0.3162162162);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "weight[2]", 0.0702702703);

	bi->context = VuoGlContext_use();

	bi->imageRenderer = VuoImageRenderer_make(bi->context);
	VuoRetain(bi->imageRenderer);

	return (VuoImageBlur)bi;
}

/**
 * Returns a blurred copy of @a image.  (Or, if radius is zero, returns @a image.)
 *
 * If @a expandBounds is true, the output image will be expanded to encompass the full bleed of the blur.
 * Otherwise, the output image will have the same dimensions as the soure image.
 */
VuoImage VuoImageBlur_blur(VuoImageBlur blur, VuoImage image, VuoReal radius, VuoBoolean expandBounds)
{
	if (!image)
		return NULL;

	if (radius < 0.0001)
		return image;

	VuoImageBlur_internal *bi = (VuoImageBlur_internal *)blur;

	int inset = expandBounds ? ceil(radius) : 0;

	// @@@ Currently it looks like the blur actually blurs more than the specified radius..?
	// inset += 2;

	int w = image->pixelsWide + inset*2;
	int h = image->pixelsHigh + inset*2;

	VuoImage img = image;

	VuoShader_setUniform_VuoReal(bi->verticalPassShader,   "inset",  inset);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader,   "width",  w);
	VuoShader_setUniform_VuoReal(bi->verticalPassShader,   "height", h);
	VuoShader_setUniform_VuoReal(bi->horizontalPassShader, "width",  w);

	for(int i = 0; i < radius; i++)
	{
		// apply vertical pass
		VuoShader_setUniform_VuoImage(bi->verticalPassShader, "texture", img);

		VuoImage verticalPassImage = VuoImageRenderer_draw(bi->imageRenderer, bi->verticalPassShader, w, h, VuoImage_getColorDepth(img));
		VuoImage_setWrapMode(verticalPassImage, VuoImageWrapMode_None);

		// Only inset the first pass
		if (inset)
		{
			inset=0;
			VuoShader_setUniform_VuoReal(bi->verticalPassShader, "inset", inset);
		}

		// apply horizontal pass
		VuoShader_setUniform_VuoImage(bi->horizontalPassShader, "texture", verticalPassImage);

		// one pass complete, ready for another (or not)
		img = VuoImageRenderer_draw(bi->imageRenderer, bi->horizontalPassShader, w, h, VuoImage_getColorDepth(img));
		VuoImage_setWrapMode(img, VuoImageWrapMode_None);
	}

	return img;
}
