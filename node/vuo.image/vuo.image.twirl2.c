/**
 * @file
 * vuo.image.twirl node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Twirl Image",
					 "keywords" : [ "twist", "swirl", "spin", "whirl", "pivot", "swivel", "revolve", "rotate", "curl", "coil", "filter" ],
					 "version": "3.0.0",
					 "dependencies" : [
						 "VuoImageRenderer"
					 ],
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	// Inputs
	uniform sampler2D texture;
	uniform vec2 center;
	uniform float angle;
	uniform float cutoffRadius;
	varying vec2 fragmentTextureCoordinate;

	void main()
	{
		// Adapted from http://www.geeks3d.com/20110428/shader-library-swirl-post-processing-filter-in-glsl/
		vec2 coord = fragmentTextureCoordinate;
		if (cutoffRadius > 0)
		{
			coord -= center;
			float radiusFromCenter = length(coord);
			if (radiusFromCenter < cutoffRadius)
			{
				float percentFromCenterToCutoff = (cutoffRadius - radiusFromCenter) / cutoffRadius;
				float theta = pow(percentFromCenterToCutoff,3.) * angle;
				float st = sin(theta);
				float ct = cos(theta);
				coord = vec2(coord.x*ct-coord.y*st, coord.x*st+coord.y*ct);
			}
			coord += center;
		}
		gl_FragColor = VuoGlsl_sample(texture, coord);
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

	instance->shader = VuoShader_make("Twirl Image");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0},"suggestedMin":{"x":-1,"y":-1},"suggestedMax":{"x":1,"y":1}}) center,
		VuoInputData(VuoReal, {"default":135.0,"suggestedMin":-360,"suggestedMax":360,"suggestedStep":1}) angle,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":2}) radius,
		VuoOutputData(VuoImage) twirledImage
)
{
	if (! image)
	{
		*twirledImage = NULL;
		return;
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "angle", -angle*M_PI/180.);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "cutoffRadius", VuoShader_samplerSizeFromVuoSize(radius));

	// Render.
	*twirledImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
