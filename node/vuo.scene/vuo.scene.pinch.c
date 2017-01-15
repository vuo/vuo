/**
 * @file
 * vuo.scene.pinch node implementation.
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
					 "title" : "Pinch 3D Object",
					 "keywords" : [ "warp", "curve", "gravitation", "gravity", "pull", "push", "attract", "repel", "bow", "flex", "displace", "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "PinchSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float amount;
	uniform vec3 center;

	vec3 deform(vec3 position)
	{
		float d = distance(center, position);
		d = amount / (d*d);
		return vec3(position.x + (center.x-position.x)*d,
					position.y + (center.y-position.y)*d,
					position.z + (center.z-position.z)*d);
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

	instance->shader = VuoShader_make("Pinch Object");
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
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoOutputData(VuoSceneObject) pinchedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal   ((*instance)->shader, "amount", amount);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center", center);

	// Render.
	*pinchedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
