/**
 * @file
 * vuo.scene.bend node implementation.
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
					 "title" : "Bend 3D Object",
					 "keywords" : [ "warp", "curve", "angle", "hook", "bow", "flex", "displace", "filter" ],
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "BendSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"

	// Inputs
	uniform float amount;
	uniform float radius;
	uniform float direction;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		float theta = position.y * amount;
		float cd = cos(direction);
		float sd = sin(direction);
		float ct = cos(theta);
		float st = sin(theta);

		// Rotate by direction about the Y axis.
		float tx = position.x * cd - position.z * sd;
		float ty = position.y;
		float tz = position.x * sd + position.z * cd;

		tx += radius;

		// Perform the rotation by theta about the Z Axis.
		float tx3 = tx * ct - ty * st;
		float ty3 = tx * st + ty * ct;

		tx3 -= radius;

		// Rotate by -direction about the Y axis.
		float tx4 = tx3 * cd  - tz * -sd;
		float tz4 = tx3 * -sd + tz * cd;

		return vec3(tx4,ty3,tz4);
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

	instance->shader = VuoShader_make("Bend Object Shader");
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
		VuoInputData(VuoReal, {"default":30.0,"suggestedMin":-120.0,"suggestedMax":120.0,"suggestedStep":15.0}) amount,
		VuoInputData(VuoReal, {"default":0.5,"suggestedMin":0.0,"suggestedMax":1.0}) radius,
		VuoInputData(VuoReal, {"default":0.0,"suggestedMin":0.0,"suggestedMax":360.0,"suggestedStep":15.0}) direction,
		VuoOutputData(VuoSceneObject) bentObject
)
{
	VuoReal normalizedAmount = fabs(amount);
	VuoReal normalizedAmountRadians = normalizedAmount * M_PI / 180.;
	VuoReal rectifiedDirection = direction + (amount<0?0:180);
	VuoReal rectifiedDirectionRadians = rectifiedDirection * M_PI / 180.;

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "amount", normalizedAmountRadians);
	VuoShader_setUniform_VuoReal((*instance)->shader, "radius", radius);
	VuoShader_setUniform_VuoReal((*instance)->shader, "direction", rectifiedDirectionRadians);

	VuoSceneObjectRenderer_CPUGeometryOperator cpuGeometryOperator = VuoSceneObjectRenderer_makeDeformer(^(VuoPoint3d position, VuoPoint3d normal, VuoPoint2d textureCoordinate) {
		float theta = position.y * normalizedAmountRadians;
		float cd    = cosf(rectifiedDirectionRadians);
		float sd    = sinf(rectifiedDirectionRadians);
		float ct    = cosf(theta);
		float st    = sinf(theta);

		// Rotate by direction about the Y axis.
		float tx = position.x * cd - position.z * sd;
		float ty = position.y;
		float tz = position.x * sd + position.z * cd;

		tx += radius;

		// Perform the rotation by theta about the Z Axis.
		float tx3 = tx * ct - ty * st;
		float ty3 = tx * st + ty * ct;

		tx3 -= radius;

		// Rotate by -direction about the Y axis.
		float tx4 = tx3 * cd - tz * -sd;
		float tz4 = tx3 * -sd + tz * cd;

		return (VuoPoint3d){ tx4,
							 ty3,
							 tz4 };
	});

	// Render.
	*bentObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object, cpuGeometryOperator);

	Block_release(cpuGeometryOperator);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
