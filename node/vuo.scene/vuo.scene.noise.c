/**
 * @file
 * vuo.scene.noise node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"
#include "VuoGradientNoiseCommon.h"

#include "VuoGlPool.h"
#include <Block.h>
#include <OpenGL/CGLMacro.h>

#include "VuoDispersion.h"
#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Add Noise to 3D Object",
					 "keywords" : [ "perturb", "random", "pseudorandom", "natural", "organic", "displace", "filter" ],
					 "version" : "1.0.1",
					 "node": {
						 "exampleCompositions" : [ "AddNoiseToClay.vuo" ]
					 },
					 "dependencies" : [
						 "VuoGradientNoiseCommon",
						 "VuoSceneObjectRenderer"
					 ]
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"
	\n#include "noise4D.glsl"

	// Inputs
	uniform vec3 amount;
	uniform float scale;
	uniform float time;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		return position + amount * snoise4D3D(vec4(position.x*scale, position.y*scale, position.z*scale, time));
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

	instance->shader = VuoShader_make("Add Noise to Object");
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
		VuoInputData(VuoReal, {"default":0.0,"suggestedStep":0.1}) time,
		VuoInputData(VuoPoint3d, {"default":{"x":0.2,"y":0.2,"z":0.2},"suggestedMin":{"x":0.0,"y":0.0,"z":0.0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) amount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) scale,
		VuoOutputData(VuoSceneObject) noisedObject
)
{
	double inverseScale = 1 / VuoReal_makeNonzero(scale);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "time",   time);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "amount", amount);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",  inverseScale);

	VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator = VuoSceneObjectRenderer_makeDeformer(^(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate) {
		VuoPoint3d noise = VuoGradientNoise_simplex_VuoPoint4d_VuoPoint3d((VuoPoint4d){
			position.x * inverseScale,
			position.y * inverseScale,
			position.z * inverseScale,
			time});
		return (VuoPoint3d){ position.x + amount.x * noise.x,
							 position.y + amount.y * noise.y,
							 position.z + amount.z * noise.z };
	});

	// Render.
	*noisedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object, cpuGeometryOperator);

	Block_release(cpuGeometryOperator);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
