/**
 * @file
 * vuo.scene.displace.image node implementation.
 *
 * @copyright Copyright © 2012–2018 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>
#include "VuoImageWrapMode.h"
#include "../vuo.image/VuoThresholdType.h"

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
	include(deform)
	include(VuoGlslBrightness)

	// Inputs
	uniform float amount;
	uniform int brightnessType;
	uniform sampler2D heightmap;
	attribute vec4 normal;

	float sum(vec3 invec)
	{
		return invec.x + invec.y + invec.z;
	}

	vec3 deform(vec3 position)
	{
		// position is in world space, so also translate the normal into world
		vec3 nrm = normalize( mat4to3(modelviewMatrix) * normal.xyz );

		vec4 pixel = texture2D(heightmap, textureCoordinate.xy);
		float grayscale = VuoGlsl_brightness(pixel, brightnessType);

		vec3 transformed = position + (nrm * grayscale * amount);

		return transformed;
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

	*deformedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
