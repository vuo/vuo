/**
 * @file
 * VuoImageWatermark
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#import "VuoFont.h"
#import "VuoImageRenderer.h"
#import "VuoImageText.h"
#import "VuoShader.h"
#import "VuoHeap.h"

/**
 * Returns a new image consisting of the input image with tiled "Vuo Free Trial" text overlaid on it.
 */
static inline VuoImage VuoImage_watermark(VuoImage image)
{
	// Render the watermark text.
	VuoImage textImage;
	{
		VuoFont font = VuoFont_make(VuoText_make("HelveticaNeue-Bold"), 72, false, VuoColor_makeWithRGBA(.1,.1,.1,1), VuoHorizontalAlignment_Left, 1, 1);
		textImage = VuoImage_makeText(" Vuo Free Trial  \n", font, 1);
		VuoRetain(textImage);
	}

	// Blend the tiled watermark text atop the composition image.
	VuoImage watermarkedImage;
	{
		static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
			uniform sampler2D background;
			uniform sampler2D foreground;
			uniform vec2 scale;
			varying vec4 fragmentTextureCoordinate;
			void main()
			{
				vec4 base = texture2D(background, fragmentTextureCoordinate.xy);
				vec4 blend = texture2D(foreground, mod(fragmentTextureCoordinate.xy / scale, 1.));
				gl_FragColor = base + blend;
			}
		);

		VuoShader shader = VuoShader_make("Watermark Shader");
		VuoRetain(shader);
		VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);

		VuoShader_setUniform_VuoImage  (shader, "background", image);
		VuoShader_setUniform_VuoImage  (shader, "foreground", textImage);
		VuoPoint2d scale = VuoPoint2d_make((double)textImage->pixelsWide/image->pixelsWide,
										   (double)textImage->pixelsHigh/image->pixelsHigh);
		VuoShader_setUniform_VuoPoint2d(shader, "scale", scale);

		VuoGlContext glContext = VuoGlContext_use();
		VuoImageRenderer imageRenderer = VuoImageRenderer_make(glContext);
		VuoRetain(imageRenderer);

		watermarkedImage = VuoImageRenderer_draw(imageRenderer, shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
		VuoGlContext_disuse(glContext);
		VuoRelease(imageRenderer);
		VuoRelease(shader);
	}

	VuoRelease(textImage);
	return watermarkedImage;
}
