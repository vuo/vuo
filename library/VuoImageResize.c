/**
 * @file
 * VuoImageResize implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageResize.h"
#include "../node/vuo.image/VuoSizingMode.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "VuoImageResize",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ]
				 });
#endif
/// @}

/**
 * Fragment shader that scales and offsets an image.
 */
static const char * applyScaleFragmentShader = VUOSHADER_GLSL_SOURCE(120,

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 scale;
	uniform vec2 offset;

	bool outOfBounds(vec2 coord)
	{
		return coord.x < 0 || coord.x > 1 || coord.y < 0 || coord.y > 1;
	}

	void main(void)
	{
		vec2 uv = (vec2(fragmentTextureCoordinate.x, fragmentTextureCoordinate.y)-offset) * scale;
		gl_FragColor = outOfBounds(uv) ? vec4(0.,0.,0.,0.) : texture2D(texture, uv);
	}
);

/**
 * Create and compile a resize shader for reuse with VuoImage_resizeWithShaderAndContext.
 */
VuoShader VuoImageResize_makeShader()
{
	VuoShader shader = VuoShader_make("Resize Image Shader");
	VuoShader_addSource(shader, VuoMesh_IndividualTriangles, NULL, NULL, applyScaleFragmentShader);
	return shader;
}

/**
 * Returns a new image resized from image.  Use with VuoImageResize_makeShader().
 */
VuoImage VuoImageResize_resize(VuoImage image, VuoShader shader, VuoImageRenderer renderer, VuoSizingMode sizingMode, VuoInteger width, VuoInteger height)
{
	if (!image)
		return NULL;

	float u = width / (float)(image->pixelsWide);
	float v = height / (float)(image->pixelsHigh);

	VuoPoint2d scale = (VuoPoint2d) { 1, 1 };
	VuoPoint2d offset = (VuoPoint2d) { 0, 0 };

	switch(sizingMode)
	{
		case VuoSizingMode_Stretch:
			break;

		case VuoSizingMode_Fit:
			if( u < v && u * image->pixelsHigh < height)
			{
				scale = (VuoPoint2d) { 1, height/(image->pixelsHigh*u) };
				offset = (VuoPoint2d) { 0, ((height-(image->pixelsHigh*u))/2)/height };
			}
			else
			{
				scale = (VuoPoint2d) { width/(image->pixelsWide*v), 1 };
				offset = (VuoPoint2d) { ((width-(image->pixelsWide*v))/2)/width, 0 };
			}
			break;

		case VuoSizingMode_Fill:
			if(u > v)
			{
				scale = (VuoPoint2d) { 1, height/(image->pixelsHigh*u) };
				offset = (VuoPoint2d) { 0, ((height-(image->pixelsHigh*u))/2)/height };
			}
			else
			{
				scale = (VuoPoint2d) { width/(image->pixelsWide*v), 1 };
				offset = (VuoPoint2d) { ((width-(image->pixelsWide*v))/2)/width, 0 };
			}
			break;
	}

	VuoShader_setUniform_VuoImage  ( shader, "texture", image );
	VuoShader_setUniform_VuoPoint2d( shader, "scale", scale );
	VuoShader_setUniform_VuoPoint2d( shader, "offset", offset );

	return VuoImageRenderer_draw( renderer, shader, width, height, VuoImage_getColorDepth(image) );
}
