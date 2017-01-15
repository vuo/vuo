/**
 * @file
 * vuo.image.bulge node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
					 "title" : "Bulge Image",
					 "keywords" : [ "push", "pull", "pinch", "outward", "inward", "bump", "swell", "lump", "magnify", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoImageRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "BulgeCheckerboard.vuo" ]
					 }
				 });


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform vec2 center;
	uniform float scale;
	uniform float cutoffRadius;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vec2 coord = fragmentTextureCoordinate.xy;
		if (cutoffRadius > 0)
		{
			coord -= center;
			coord.y /= aspectRatio;
			float radiusFromCenter = length(coord);
			if (radiusFromCenter < cutoffRadius)
			{
				float percentFromCenterToCutoff = (cutoffRadius - radiusFromCenter) / cutoffRadius;

				float bulge = radiusFromCenter;
				bulge = mix(1., bulge, smoothstep(0., 1., percentFromCenterToCutoff));
				bulge = mix(1., bulge, scale);
				bulge = pow(bulge, 2.);

				coord *= vec2(bulge, bulge);
			}
			coord.y *= aspectRatio;
			coord += center;
		}
		gl_FragColor = texture2D(texture, coord);
	}
);


struct nodeInstanceData
{
	VuoShader shader;
	VuoGlContext glContext;
	VuoImageRenderer imageRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Bulge Image");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	instance->glContext = VuoGlContext_use();

	instance->imageRenderer = VuoImageRenderer_make(instance->glContext);
	VuoRetain(instance->imageRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint2d, {"default":{"x":0,"y":0},"suggestedMin":{"x":-1,"y":-1},"suggestedMax":{"x":1,"y":1}}) center,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":-0.5,"suggestedMax":1.0,"suggestedStep":0.1}) scale,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0,"suggestedMax":2}) radius,
		VuoOutputData(VuoImage) bulgedImage
)
{
	if (! image)
		return;

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoPoint2d((*instance)->shader, "center", VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale", scale);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "cutoffRadius", VuoShader_samplerSizeFromVuoSize(radius));

	// Render.
	*bulgedImage = VuoImageRenderer_draw((*instance)->imageRenderer, (*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->imageRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
