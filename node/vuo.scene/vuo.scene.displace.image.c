/**
 * @file
 * vuo.scene.displace.image node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
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
					 "keywords" : [ "heightmap", "rutt", "etra", "normal", "bump", "topology", "morph" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
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

	// Inputs
	uniform float amount;
	uniform vec4 channel;
	uniform sampler2D heightmap;

	float sum(vec3 invec)
	{
		return invec.x + invec.y + invec.z;
	}

	vec3 deform(vec3 position)
	{
		// position is in world space, so also translate the normal into world
		vec3 nrm = normalize( mat4to3(modelviewMatrix) * normal.xyz );

		vec4 pixel = texture2D(heightmap, textureCoordinate.xy);
		float mask_sum = sum(channel.rgb);
		float grayscale = mask_sum > 0 ? sum((pixel.rgb * pixel.a) * channel.rgb) / mask_sum : pixel.a / 1.;

		vec3 transformed = position + (nrm * grayscale * amount);

		return transformed;
	}
);

struct nodeInstanceData
{
	VuoShader shader;

	VuoGlContext glContext;
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

	instance->glContext = VuoGlContext_use();

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->glContext, instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoImage) image,
		VuoInputData(VuoThresholdType, {"default":"luminance"}) channel,
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
	VuoPoint4d mask = VuoPoint4d_make( 	channel == VuoThresholdType_Luminance || channel == VuoThresholdType_Red ? 1 : 0,
										channel == VuoThresholdType_Luminance || channel == VuoThresholdType_Green ? 1 : 0,
										channel == VuoThresholdType_Luminance || channel == VuoThresholdType_Blue ? 1 : 0,
										channel == VuoThresholdType_Luminance || channel == VuoThresholdType_Alpha ? 1 : 0 );

	VuoShader_setUniform_VuoPoint4d((*instance)->shader, "channel", mask);
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", distance);
	VuoShader_setUniform_VuoImage((*instance)->shader, "heightmap", image);

	*deformedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
