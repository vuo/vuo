/**
 * @file
 * vuo.image.tile node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Tile Image",
					  "keywords" : [ "reflect", "mirror", "flip", "rotate", "linear", "affine", "wrap", "copy", "filter" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "TileMovie.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	include(VuoGlslAlpha)

	varying vec4 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform vec2 center;
	uniform float scale;
	uniform vec2 imageSize;
	uniform bool reflectOddRows;
	uniform bool reflectOddColumns;

	void main(void)
	{
		vec2 tiledPosition = fragmentTextureCoordinate.xy;

		tiledPosition -= center;

		tiledPosition *= scale;

		// Wrap at slightly less than 1 to avoid a transparent line between tiles.
		vec2 wrapCorner = 1. - 1./imageSize;

		if (reflectOddRows && (mod(tiledPosition.y, wrapCorner.y*2.) < wrapCorner.y))
			tiledPosition.y = wrapCorner.y - tiledPosition.y;

		if (reflectOddColumns && (mod(tiledPosition.x, wrapCorner.x*2.) < wrapCorner.x))
			tiledPosition.x = wrapCorner.x - tiledPosition.x;

		tiledPosition = mod(tiledPosition, wrapCorner) + 0.5 / imageSize;

		gl_FragColor = VuoGlsl_sample(texture, tiledPosition);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Tile Image Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) center,
		VuoInputData(VuoReal, {"default":0.5, "suggestedMin":0., "suggestedMax":2., "suggestedStep":0.1}) width,
		VuoInputData(VuoBoolean, {"default":false}) reflectOddRows,
		VuoInputData(VuoBoolean, {"default":false}) reflectOddColumns,
		VuoOutputData(VuoImage) tiledImage
)
{
	if (!image)
	{
		*tiledImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture",           image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "imageSize",         VuoPoint2d_make(image->pixelsWide, image->pixelsHigh));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center",            VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",             1./VuoReal_makeNonzero(VuoShader_samplerSizeFromVuoSize(width)));
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "reflectOddRows",    reflectOddRows);
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "reflectOddColumns", reflectOddColumns);

	*tiledImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
