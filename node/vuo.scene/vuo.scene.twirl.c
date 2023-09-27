/**
 * @file
 * vuo.scene.twirl node implementation.
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
					 "title" : "Twirl 3D Object",
					 "keywords" : [ "warp", "curve", "angle",
						 "curl", "screw", "wring", "pivot", "turn", "swivel", "spin", "rotate", "twist",
						 "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "TwirlGrid.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"

	// Inputs
	uniform float amount;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		float theta = position.y * amount;
		float ct = cos(theta);
		float st = sin(theta);
		return vec3(position.x * ct - position.z * st, position.y, position.x * st + position.z * ct);
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

	instance->shader = VuoShader_make("Twirl Object");
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
		VuoInputData(VuoReal, {"default":180.0,"suggestedMin":-360.0,"suggestedMax":360.0,"suggestedStep":15.0}) amount,
		VuoOutputData(VuoSceneObject) twirledObject
)
{
	VuoReal amountRadians = amount * M_PI / 180.;

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", amountRadians);

	VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator = VuoSceneObjectRenderer_makeDeformer(^(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate) {
		float theta = position.y * amountRadians;
		float ct = cosf(theta);
		float st = sinf(theta);
		return (VuoPoint3d){position.x * ct - position.z * st,
							position.y,
							position.x * st + position.z * ct};
	});

	// Render.
	*twirledObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object, cpuGeometryOperator);

	Block_release(cpuGeometryOperator);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
