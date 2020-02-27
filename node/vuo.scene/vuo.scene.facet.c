/**
 * @file
 * vuo.scene.facet node implementation.
 *
 * @copyright Copyright © 2012–2020 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Facet 3D Object",
					 "keywords" : [
						 "sharpen", "harden", "flatten",
						 "edge", "side",
						 "filter"
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "FacetSphere.vuo" ]
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

		// Emit the vertices with the calculated normal.
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i];
			outNormal = normal;
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

	instance->shader = VuoShader_make("Facet Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource,          geometryShaderSource, NULL);

	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoSceneObject) facetedObject
)
{
	*facetedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			if (*vertexCount != 3)
				return;

			// Calculate two vectors in the plane of the input triangle.
			VuoPoint3d position0 = VuoPoint3d_makeFromArray(&positions[0 * 3]);
			VuoPoint3d position1 = VuoPoint3d_makeFromArray(&positions[1 * 3]);
			VuoPoint3d position2 = VuoPoint3d_makeFromArray(&positions[2 * 3]);
			VuoPoint3d ab = position1 - position0;
			VuoPoint3d ac = position2 - position0;
			VuoPoint3d normal = VuoPoint3d_normalize(VuoPoint3d_crossProduct(ab, ac));
			for (int i = 0; i < 3; ++i)
				VuoPoint3d_setArray(&normals[i * 3], normal);
		});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
