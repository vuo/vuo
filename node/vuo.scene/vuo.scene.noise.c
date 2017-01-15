/**
 * @file
 * vuo.scene.noise node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

#include "VuoDispersion.h"
#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Add Noise to 3D Object",
					 "keywords" : [ "perturb", "random", "pseudorandom", "natural", "displace", "filter" ],
					 "version" : "1.0.1",
					 "node": {
						 "exampleCompositions" : [ "AddNoiseToClay.vuo" ]
					 },
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ]
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)
	include(noise4D)

	// Inputs
	uniform vec3 amount;
	uniform float scale;
	uniform float time;

	vec3 deform(vec3 position)
	{
		return position + amount * snoise4D3D(vec4(position.x*scale, position.y*scale, position.z*scale, time));
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

	instance->shader = VuoShader_make("Add Noise to Object");
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
		VuoInputData(VuoReal, {"default":0.0,"suggestedStep":0.1}) time,
		VuoInputData(VuoPoint3d, {"default":{"x":0.2,"y":0.2,"z":0.2},"suggestedMin":{"x":0.0,"y":0.0,"z":0.0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) amount,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedMax":2.0}) scale,
		VuoOutputData(VuoSceneObject) noisedObject
)
{
	double nonzeroScale = VuoReal_makeNonzero(scale);

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "time",   time);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "amount", amount);
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "scale",  1/nonzeroScale);

	// Render.
	*noisedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
