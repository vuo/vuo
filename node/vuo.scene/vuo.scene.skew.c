/**
 * @file
 * vuo.scene.skew node implementation.
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
					 "title" : "Skew 3D Object",
					 "keywords" : [ "shear", "lean", "angle", "slant", "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SkewSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float amount;
	uniform float direction;

	vec3 deform(vec3 position)
	{
		float bend = position.y * amount;
		return vec3(position.x + bend*cos(direction), position.y, position.z + bend*sin(direction));
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

	instance->shader = VuoShader_make("Skew Object");
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
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":-1.0,"suggestedMax":1.0,"suggestedStep":0.1}) amount,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":360.0,"suggestedStep":15.0}) direction,
		VuoOutputData(VuoSceneObject) skewedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", amount);
	VuoShader_setUniform_VuoReal((*instance)->shader, "direction", -direction*M_PI/180.);

	// Render.
	*skewedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
