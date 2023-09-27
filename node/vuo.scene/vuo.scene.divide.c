/**
 * @file
 * vuo.scene.divide node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <Block.h>
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Divide 3D Object",
					 "keywords" : [ "break", "separate", "split", "slice", "face", "edge", "side", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "DivideSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"

	// Inputs
	uniform float dist;

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		// Simplified, origin-based version for point and line meshes, since they don't have normals.
		float l = length(position);
		return position + dist * l * l * sign(position);
	}
);

static const char *triangleVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	// Outputs
	varying vec3 geometryPosition;
	varying vec3 geometryNormal;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

	void main()
	{
		geometryPosition = position;
		geometryNormal = normal;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = vertexColor;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *triangleGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec3 geometryPosition[3];
	varying in vec3 geometryNormal[3];
	varying in vec2 geometryTextureCoordinate[3];
	varying in vec4 geometryVertexColor[3];
	uniform float dist;

	// Outputs
	varying out vec3 outPosition;
	varying out vec3 outNormal;
	varying out vec2 outTextureCoordinate;
	varying out vec4 outVertexColor;

	void main()
	{
		// Calculate two vectors in the plane of the input triangle.
		vec3 ab = geometryPosition[1] - geometryPosition[0];
		vec3 ac = geometryPosition[2] - geometryPosition[0];
		vec3 normal = normalize(cross(ab, ac));

		// Emit the vertices with the recalculated position.
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i] + normal.xyz * dist;
			outNormal = geometryNormal[i];
			outTextureCoordinate = geometryTextureCoordinate[i];
			outVertexColor = geometryVertexColor[i];
			EmitVertex();
		}
		EndPrimitive();
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

	instance->shader = VuoShader_make("Divide Object");
	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                         NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                         NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, triangleVertexShaderSource,  triangleGeometryShaderSource, NULL);
	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0.0,"suggestedMax":1.0}) distance,
		VuoOutputData(VuoSceneObject) dividedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "dist", distance);

	// Render.
	*dividedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			if (*vertexCount < 3)
			{
				// Simplified, origin-based version for point and line meshes, since they don't have normals.
				for (int i = 0; i < *vertexCount; ++i)
				{
					VuoPoint3d position = VuoPoint3d_makeFromArray(&positions[i * 3]);
					float l = VuoPoint3d_magnitude(position);
					positions[i * 3    ] += distance * l * l * (position.x > 0 ? 1 : -1);
					positions[i * 3 + 1] += distance * l * l * (position.y > 0 ? 1 : -1);
					positions[i * 3 + 2] += distance * l * l * (position.z > 0 ? 1 : -1);
				}
			}
			else
			{
				// Calculate two vectors in the plane of the input triangle.
				VuoPoint3d position0 = VuoPoint3d_makeFromArray(&positions[0 * 3]);
				VuoPoint3d position1 = VuoPoint3d_makeFromArray(&positions[1 * 3]);
				VuoPoint3d position2 = VuoPoint3d_makeFromArray(&positions[2 * 3]);
				VuoPoint3d ab = position1 - position0;
				VuoPoint3d ac = position2 - position0;
				VuoPoint3d normal = VuoPoint3d_normalize(VuoPoint3d_crossProduct(ab, ac));

				for (int i = 0; i < 3; ++i)
				{
					positions[i * 3    ] += normal.x * distance;
					positions[i * 3 + 1] += normal.y * distance;
					positions[i * 3 + 2] += normal.z * distance;
				}
			}
		});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
