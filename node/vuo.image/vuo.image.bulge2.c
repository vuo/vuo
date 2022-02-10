/**
 * @file
 * vuo.image.bulge2 node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoImageRenderer.h"

VuoModuleMetadata({
	"title" : "Bulge Image",
	"keywords" : [
		"push", "pull", "pinch", "outward", "inward", "bump", "swell", "lump", "magnify", "filter",
	],
	"version" : "2.0.0",
	"dependencies" : [
		"VuoImageRenderer",
	],
});


static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "VuoGlslAlpha.glsl"

	// Inputs
	uniform sampler2D texture;
	uniform float aspectRatio;
	uniform int bulgeCount;
	uniform vec2[64] centers;
	uniform float[64] scales;
	uniform float[64] radii;
	varying vec2 fragmentTextureCoordinate;

	void main()
	{
		vec2 coord = fragmentTextureCoordinate;
		for (int i = 0; i < bulgeCount; ++ i)
		{
			coord -= centers[i];
			coord.y /= aspectRatio;
			float radiusFromCenter = length(coord);
			if (radiusFromCenter < radii[i])
			{
				float percentFromCenterToCutoff = (radii[i] - radiusFromCenter) / radii[i];

				float bulge = radiusFromCenter;
				bulge = mix(1., bulge, smoothstep(0., 1., percentFromCenterToCutoff));
				bulge = mix(1., bulge, scales[i]);
				bulge = pow(bulge, 2.);

				coord *= vec2(bulge, bulge);
			}
			coord.y *= aspectRatio;
			coord += centers[i];
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

	instance->shader = VuoShader_make("Bulge Image");
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, NULL, NULL, fragmentShaderSource);
	VuoRetain(instance->shader);

	return instance;
}

void nodeInstanceEvent
(
	VuoInstanceData(struct nodeInstanceData *) instance,
	VuoInputData(VuoImage) image,
	VuoInputData(VuoList_VuoPoint2d, {"default":[{"x":-0.3,"y":0.3},{"x":0.3,"y":-0.3}],"suggestedMin":{"x":-1,"y":-1},"suggestedMax":{"x":1,"y":1}}) centers,
	VuoInputData(VuoList_VuoReal, {"default":[0.5,0.5],"suggestedMin":-0.5,"suggestedMax":1.0,"suggestedStep":0.1}) scales,
	VuoInputData(VuoList_VuoReal, {"default":[0.5,0.5],"suggestedMin":0,"suggestedMax":2}) radii,
	VuoOutputData(VuoImage) bulgedImage
)
{
	if (!image)
	{
		*bulgedImage = NULL;
		return;
	}

	unsigned int centerCount = VuoListGetCount_VuoPoint2d(centers),
				 scaleCount  = VuoListGetCount_VuoReal(scales),
				 radiusCount = VuoListGetCount_VuoReal(radii);
	if (!centerCount || !scaleCount || !radiusCount)
	{
		*bulgedImage = image;
		return;
	}

	VuoList_VuoPoint2d shaderCenters = VuoListCreate_VuoPoint2d();
	VuoList_VuoReal shaderScales = VuoListCreate_VuoReal();
	VuoList_VuoReal shaderRadii = VuoListCreate_VuoReal();
	VuoLocal(shaderCenters);
	VuoLocal(shaderScales);
	VuoLocal(shaderRadii);
	unsigned int bulgeCount = MAX(centerCount, MAX(scaleCount, radiusCount));
	for(int i = 0; i < bulgeCount; i++)
	{
		VuoPoint2d center = VuoListGetValue_VuoPoint2d(centers, i + 1);
		VuoReal    scale  = VuoListGetValue_VuoReal   (scales,  i + 1);
		VuoReal    radius = VuoListGetValue_VuoReal   (radii,   i + 1);

		// Linearly extrapolate.
		if (i >= centerCount)
			center =
				VuoPoint2d_add(center,
					VuoPoint2d_multiply(
						VuoPoint2d_subtract(center, VuoListGetValue_VuoPoint2d(centers, centerCount - 1)),
						i - (centerCount - 1)
					)
				);

		if (i >= scaleCount)
			scale = (scale + (scale - VuoListGetValue_VuoReal(scales, scaleCount - 1)) * (i - (scaleCount - 1)));

		if (i >= radiusCount)
			radius = (radius + (radius - VuoListGetValue_VuoReal(radii, radiusCount - 1)) * (i - (radiusCount - 1)));

		VuoListAppendValue_VuoPoint2d(shaderCenters, VuoShader_samplerCoordinatesFromVuoCoordinates(center, image));
		VuoListAppendValue_VuoReal   (shaderScales,  scale);
		VuoListAppendValue_VuoReal   (shaderRadii,   VuoShader_samplerSizeFromVuoSize(radius));
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoImage  ((*instance)->shader, "texture", image);
	VuoShader_setUniform_VuoInteger((*instance)->shader, "bulgeCount", bulgeCount);

	VuoShader_setUniform_VuoList_VuoPoint2d((*instance)->shader, "centers", shaderCenters);
	VuoShader_setUniform_VuoList_VuoReal   ((*instance)->shader, "scales",  shaderScales);
	VuoShader_setUniform_VuoList_VuoReal   ((*instance)->shader, "radii",   shaderRadii);

	// Render.
	*bulgedImage = VuoImageRenderer_render((*instance)->shader, image->pixelsWide, image->pixelsHigh, VuoImage_getColorDepth(image));
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
}
