/**
 * @file
 * vuo.scene.skew node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <Block.h>
#include <OpenGL/CGLMacro.h>

#include "VuoDispersion.h"
#include "VuoDisplacement.h"

VuoModuleMetadata({
					 "title" : "Skew 3D Object",
					 "keywords" : [ "shear", "lean", "angle", "slant", "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SkewSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"

	// Inputs
	uniform float amount;
	uniform float direction;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		float bend = position.y * amount;
		return vec3(position.x + bend*cos(direction), position.y, position.z + bend*sin(direction));
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

	instance->shader = VuoShader_make("Skew Object");
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
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":-1.0,"suggestedMax":1.0,"suggestedStep":0.1}) amount,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":360.0,"suggestedStep":15.0}) direction,
		VuoOutputData(VuoSceneObject) skewedObject
)
{
	VuoReal directionRadians = -direction * M_PI / 180.;

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", amount);
	VuoShader_setUniform_VuoReal((*instance)->shader, "direction", directionRadians);

	VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator = VuoSceneObjectRenderer_makeDeformer(^(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate) {
		float bend = position.y * amount;
		return (VuoPoint3d){ position.x + bend * cosf(directionRadians),
							 position.y,
							 position.z + bend * sinf(directionRadians) };
	});

	// Render.
	*skewedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object, cpuGeometryOperator);

	Block_release(cpuGeometryOperator);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
