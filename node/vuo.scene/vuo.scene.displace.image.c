/**
 * @file
 * vuo.scene.displace.image node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <Block.h>
#include <OpenGL/CGLMacro.h>
#include "VuoImageWrapMode.h"
#include "VuoThresholdType.h"

VuoModuleMetadata({
					 "title" : "Displace 3D Object with Image",
					 "keywords" : [ "filter", "heightmap", "rutt", "etra", "normal", "bump", "topology", "morph" ],
					 "version" : "1.1.0",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [
							 "DentRoom.vuo",
							 "DisplaceRadialGradient.vuo",
							 "MakeRuggedTerrain.vuo"
						 ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"
	\n#include "VuoGlslBrightness.glsl"

	// Inputs
	uniform float amount;
	uniform int brightnessType;
	uniform sampler2D heightmap;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		vec4 pixel = texture2D(heightmap, textureCoordinate);
		float grayscale = VuoGlsl_brightness(pixel, brightnessType);

		return position + (normal * grayscale * amount);
	}
);

struct nodeInstanceData
{
	VuoShader shader;
	VuoSceneObjectRenderer sceneObjectRenderer;
};

struct nodeInstanceData * nodeInstanceInit(void)
{
	struct nodeInstanceData * instance = (struct nodeInstanceData *)malloc(sizeof(struct nodeInstanceData));
	VuoRegister(instance, free);

	instance->shader = VuoShader_make("Displace Object");
	VuoShader_addSource(instance->shader, VuoMesh_Points,              vertexShaderSource, NULL, NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     vertexShaderSource, NULL, NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource, NULL, NULL);
	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

static inline VuoColor vuo_scene_displace_image_colorAtCoordinate(const unsigned char *pixels,
						const unsigned int width,
						const unsigned int height,
						const unsigned int x,
						const unsigned int y)
{
	const unsigned char* position = &pixels[((4 * width * y)) + (x * 4)];

	float a = ((unsigned int)position[3]) * 1./255;
	if (fabs(a) > 0.000001)
	{
		// The buffer returned by VuoImage_getBuffer() has its RGB values premultiplied by A,
		// so we need to un-premultiply them here since VuoColor expects un-premultiplied values.
		return (VuoColor){
			((unsigned int)position[2]) * 1./255 / a,
			((unsigned int)position[1]) * 1./255 / a,
			((unsigned int)position[0]) * 1./255 / a,
			a};
	}
	else
		return (VuoColor){0,0,0,0};
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoThresholdType, {"default":"rec709", "name":"Brightness Type", "includeValues":["rec601","rec709","desaturate","rgb-average","rgb-minimum","rgb-maximum","red","green","blue","alpha"]}) channel, // Hide "rgb" since it isn't relevant to this node.
		VuoInputData(VuoReal, {"default":1,"suggestedMin":-2.0,"suggestedMax":2.0,"suggestedStep":0.1}) distance,
		VuoOutputData(VuoSceneObject) deformedObject
)
{
	if(!image)
	{
		*deformedObject = object;
		return;
	}

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoInteger((*instance)->shader, "brightnessType", channel);
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", distance);
	VuoShader_setUniform_VuoImage((*instance)->shader, "heightmap", image);

	const unsigned char *pixels = NULL;
	if (!VuoSceneObjectRenderer_usingGPU())
		pixels = VuoImage_getBuffer(image, GL_BGRA);

	VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator = VuoSceneObjectRenderer_makeDeformer(^(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate) {
		VuoColor pixel = vuo_scene_displace_image_colorAtCoordinate(pixels, image->pixelsWide, image->pixelsHigh,
			VuoInteger_clamp(textureCoordinate.x * image->pixelsWide, 0, image->pixelsWide-1),
			VuoInteger_clamp(textureCoordinate.y * image->pixelsHigh, 0, image->pixelsHigh-1));
		float grayscale = VuoColor_brightness(pixel, channel);
		return position + normal * (VuoPoint3d)(grayscale * distance);
	});

	*deformedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object, cpuGeometryOperator);

	Block_release(cpuGeometryOperator);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
