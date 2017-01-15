/**
 * @file
 * vuo.scene.spike node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
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
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "SpikeSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	vec3 deform(vec3 position)
	{
		// Do nothing.
		return position;
	}
);

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	attribute vec4 position;
	attribute vec4 textureCoordinate;

	// Outputs
	varying vec4 geometryPosition;
	varying vec4 geometryTextureCoordinate;

	void main()
	{
		geometryPosition = position;
		geometryTextureCoordinate = textureCoordinate;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec4 geometryPosition[3];
	varying in vec4 geometryTextureCoordinate[3];
	uniform float dist;

	// Outputs
	varying out vec4 outPosition;
	varying out vec4 outNormal;
	varying out vec4 outTangent;
	varying out vec4 outBitangent;
	varying out vec4 outTextureCoordinate;

	include(triangle)

	void main()
	{
		// Find the position and textureCoordinate in the center of the 3 vertices.
		vec3 fourthVertex = (geometryPosition[0].xyz+geometryPosition[1].xyz+geometryPosition[2].xyz)/3;
		vec4 fourthTextureCoordinate = (geometryTextureCoordinate[0]+geometryTextureCoordinate[1]+geometryTextureCoordinate[2])/3;

		// Move it inward or outward along the normal.
		vec3 ab = geometryPosition[1].xyz - geometryPosition[0].xyz;
		vec3 ac = geometryPosition[2].xyz - geometryPosition[0].xyz;
		vec3 normal = normalize(cross(ab, ac));
		fourthVertex += normal*dist;

		// Emit three triangles, each touching fourthVertex.
		vec3 p[3]; vec4 t[3];

		p[0] = geometryPosition[0].xyz; p[1] = geometryPosition[1].xyz; p[2] = fourthVertex;
		t[0] = geometryTextureCoordinate[0]; t[1] = geometryTextureCoordinate[1]; t[2] = fourthTextureCoordinate;
		emitTriangle(p,t);

		p[0] = geometryPosition[0].xyz; p[1] = fourthVertex; p[2] = geometryPosition[2].xyz;
		t[0] = geometryTextureCoordinate[0]; t[1] = fourthTextureCoordinate; t[2] = geometryTextureCoordinate[2];
		emitTriangle(p,t);

		p[0] = fourthVertex; p[1] = geometryPosition[1].xyz; p[2] = geometryPosition[2].xyz;
		t[0] = fourthTextureCoordinate; t[1] = geometryTextureCoordinate[1]; t[2] = geometryTextureCoordinate[2];
		emitTriangle(p,t);
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

	instance->shader = VuoShader_make("Spike Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                 NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                 NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource,          geometryShaderSource, NULL);
	VuoShader_setExpectedOutputPrimitiveCount(instance->shader, VuoMesh_IndividualTriangles, 3);

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
		VuoInputData(VuoReal, {"default":0.01,"suggestedMin":0.0,"suggestedMax":0.1}) distance,
		VuoOutputData(VuoSceneObject) spikedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "dist", distance);

	// Render.
	*spikedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
