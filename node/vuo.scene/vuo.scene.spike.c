/**
 * @file
 * vuo.scene.spike node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Spike 3D Object",
					 "keywords" : [ "star", "stellate", "extend", "poke", "divide", "tessellate", "face", "edge", "side", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SpikeSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	\n#include "deform.glsl"

	vec3 deform(vec3 position, vec3 normal, vec2 textureCoordinate)
	{
		// Do nothing.
		return position;
	}
);

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	attribute vec3 position;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	// Outputs
	varying vec3 geometryPosition;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

	void main()
	{
		geometryPosition = position;
		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = vertexColor;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec3 geometryPosition[3];
	varying in vec2 geometryTextureCoordinate[3];
	varying in vec4 geometryVertexColor[3];

	uniform float dist;

	// Outputs
	varying out vec3 outPosition;
	varying out vec3 outNormal;
	varying out vec2 outTextureCoordinate;
	varying out vec4 outVertexColor;

	\n#include "triangle.glsl"

	void main()
	{
		// Find the position and textureCoordinate in the center of the 3 vertices.
		vec3 fourthVertex = (geometryPosition[0]+geometryPosition[1]+geometryPosition[2])/3;
		vec2 fourthTextureCoordinate = (geometryTextureCoordinate[0]+geometryTextureCoordinate[1]+geometryTextureCoordinate[2])/3;
		vec4 fourthColor = (geometryVertexColor[0]+geometryVertexColor[1]+geometryVertexColor[2])/3;

		// Move it inward or outward along the normal.
		vec3 ab = geometryPosition[1] - geometryPosition[0];
		vec3 ac = geometryPosition[2] - geometryPosition[0];
		vec3 normal = normalize(cross(ab, ac));
		fourthVertex += normal*dist;

		// Emit three triangles, each touching fourthVertex.
		vec3 p[3]; vec2 t[3]; vec4 c[3];

		p[0] = geometryPosition[0]; p[1] = geometryPosition[1]; p[2] = fourthVertex;
		t[0] = geometryTextureCoordinate[0]; t[1] = geometryTextureCoordinate[1]; t[2] = fourthTextureCoordinate;
		c[0] = geometryVertexColor[0]; c[1] = geometryVertexColor[1]; c[2] = fourthColor;
		emitTriangle(p, t, c);

		p[0] = geometryPosition[0]; p[1] = fourthVertex; p[2] = geometryPosition[2];
		t[0] = geometryTextureCoordinate[0]; t[1] = fourthTextureCoordinate; t[2] = geometryTextureCoordinate[2];
		c[0] = geometryVertexColor[0]; c[1] = fourthColor; c[2] = geometryVertexColor[2];
		emitTriangle(p, t, c);

		p[0] = fourthVertex; p[1] = geometryPosition[1]; p[2] = geometryPosition[2];
		t[0] = fourthTextureCoordinate; t[1] = geometryTextureCoordinate[1]; t[2] = geometryTextureCoordinate[2];
		c[0] = fourthColor; c[1] = geometryVertexColor[1]; c[2] = geometryVertexColor[2];
		emitTriangle(p, t, c);
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

	instance->shader = VuoShader_make("Spike Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                 NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                 NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource,          geometryShaderSource, NULL);
	VuoShader_setExpectedOutputPrimitiveCount(instance->shader, VuoMesh_IndividualTriangles, 3);

	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

static inline void vuo_scene_spike_emitTriangle(int *vertexCount, float *positions, float *normals, float *textureCoordinates,
	VuoPoint3d p0, VuoPoint3d p1, VuoPoint3d p2,
	VuoPoint2d t0, VuoPoint2d t1, VuoPoint2d t2)
{
	VuoPoint3d ab     = p1 - p0;
	VuoPoint3d ac     = p2 - p0;
	VuoPoint3d normal = VuoPoint3d_normalize(VuoPoint3d_crossProduct(ab, ac));

	VuoPoint3d_setArray(&positions         [*vertexCount * 3], p0);
	VuoPoint3d_setArray(&normals           [*vertexCount * 3], normal);
	VuoPoint2d_setArray(&textureCoordinates[*vertexCount * 2], t0);
	++*vertexCount;

	VuoPoint3d_setArray(&positions         [*vertexCount * 3], p1);
	VuoPoint3d_setArray(&normals           [*vertexCount * 3], normal);
	VuoPoint2d_setArray(&textureCoordinates[*vertexCount * 2], t1);
	++*vertexCount;

	VuoPoint3d_setArray(&positions         [*vertexCount * 3], p2);
	VuoPoint3d_setArray(&normals           [*vertexCount * 3], normal);
	VuoPoint2d_setArray(&textureCoordinates[*vertexCount * 2], t2);
	++*vertexCount;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoReal, {"default":0.01,"suggestedMin":0.0,"suggestedMax":0.1}) distance,
		VuoOutputData(VuoSceneObject) spikedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "dist", distance);

	// Render.
	*spikedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			if (*vertexCount != 3)
				return;

			VuoPoint3d p0 = VuoPoint3d_makeFromArray(&positions[0 * 3]);
			VuoPoint3d p1 = VuoPoint3d_makeFromArray(&positions[1 * 3]);
			VuoPoint3d p2 = VuoPoint3d_makeFromArray(&positions[2 * 3]);
			VuoPoint2d t0 = VuoPoint2d_makeFromArray(&textureCoordinates[0 * 3]);
			VuoPoint2d t1 = VuoPoint2d_makeFromArray(&textureCoordinates[1 * 3]);
			VuoPoint2d t2 = VuoPoint2d_makeFromArray(&textureCoordinates[2 * 3]);

			// Find the position and textureCoordinate in the center of the 3 vertices.
			VuoPoint3d fourthVertex            = (p0 + p1 + p2) / 3.f;
			VuoPoint2d fourthTextureCoordinate = (t0 + t1 + t2) / 3.f;

			// Move it inward or outward along the normal.
			VuoPoint3d ab     = p1 - p0;
			VuoPoint3d ac     = p2 - p0;
			VuoPoint3d normal = VuoPoint3d_normalize(VuoPoint3d_crossProduct(ab, ac));
			fourthVertex = fourthVertex + normal * (VuoPoint3d)(distance);

			// Emit three triangles, each touching fourthVertex.
			*vertexCount = 0;

			vuo_scene_spike_emitTriangle(vertexCount, positions, normals, textureCoordinates,
										 p0, p1, fourthVertex,
										 t0, t1, fourthTextureCoordinate);

			vuo_scene_spike_emitTriangle(vertexCount, positions, normals, textureCoordinates,
										 p0, fourthVertex, p2,
										 t0, fourthTextureCoordinate, t2);

			vuo_scene_spike_emitTriangle(vertexCount, positions, normals, textureCoordinates,
										 fourthVertex, p1, p2,
										 fourthTextureCoordinate, t1, t2);
		});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
