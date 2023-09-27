/**
 * @file
 * vuo.image.kaleidoscope node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Make Kaleidoscope Image",
					  "keywords" : [ "reflect", "mirror", "flip", "rotate", "shard", "radial", "filter" ],
					  "version" : "1.0.1",
					  "node": {
						  "exampleCompositions" : [ "CompareKaleidoscopes.vuo" ]
					  }
				 });

static const char *fragmentShader = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	varying vec2 fragmentTextureCoordinate;
	uniform sampler2D texture;
	uniform float sides;
	uniform float angle;
	uniform vec2 bladeCenter;
	uniform vec2 imageCenter;
	uniform bool reflectOddSides;
	uniform float aspectRatio;

	void main(void)
	{
		vec2 centeredPosition = fragmentTextureCoordinate;
		centeredPosition -= bladeCenter;
		centeredPosition.y /= aspectRatio;

		float r = length(centeredPosition);
		float theta = atan(centeredPosition.y, centeredPosition.x) - angle;

		// Move the origin so there's only a single discontinuity (at theta=0) instead of two (at theta=0 and theta=π).
		if (theta < 0.)
			theta += 2. * 3.14159;

		theta = mod(theta, sides * 2.);

		if (reflectOddSides)
			theta = abs(theta - sides);

		vec2 reflectedPosition = r * vec2(cos(theta), sin(theta));
		reflectedPosition.y *= aspectRatio;
		reflectedPosition += imageCenter;
		gl_FragColor = VuoGlsl_sample(texture, reflectedPosition);
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

	instance->shader = VuoShader_make("Reflect Image Radially Shader");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShader);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":6., "suggestedMin":1., "suggestedMax":16.}) sides,
		VuoInputData(VuoReal, {"default":0., "suggestedMin":0., "suggestedMax":360.}) angle,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) bladeCenter,
		VuoInputData(VuoPoint2d, {"default":{"x":0.0,"y":0.0}, "suggestedMin":{"x":-1,"y":-1}, "suggestedMax":{"x":1,"y":1}, "suggestedStep":{"x":0.1,"y":0.1}}) imageCenter,
		VuoInputData(VuoBoolean, {"default":true}) reflectOddSides,
		VuoOutputData(VuoImage) reflectedImage
)
{
	if (!image)
	{
		*reflectedImage = NULL;
		return;
	}

	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture",          image);

	VuoReal actualSides = MAX(1, sides);
	if (reflectOddSides)
		actualSides /= 2;
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "sides",            M_PI/actualSides);

	VuoShader_setUniform_VuoReal   ((*instance)->shader, "angle",            angle * M_PI/180.);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "bladeCenter",      VuoShader_samplerCoordinatesFromVuoCoordinates(bladeCenter, image));
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "imageCenter",      VuoShader_samplerCoordinatesFromVuoCoordinates(imageCenter, image));
	VuoShader_setUniform_VuoBoolean((*instance)->shader, "reflectOddSides",  reflectOddSides);

	*reflectedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
