/**
 * @file
 * vuo.scene.facet node implementation.
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
					 "title" : "Facet 3D Object",
					 "keywords" : [ "sharpen", "harden", "edge", "side", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "FacetSphere.vuo" ]
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

	// Outputs
	varying out vec4 outPosition;
	varying out vec4 outNormal;
	varying out vec4 outTangent;
	varying out vec4 outBitangent;
	varying out vec4 outTextureCoordinate;

	void main()
	{
		// Calculate two vectors in the plane of the input triangle.
		vec3 ab = geometryPosition[1].xyz - geometryPosition[0].xyz;
		vec3 ac = geometryPosition[2].xyz - geometryPosition[0].xyz;
		vec4 normal    = vec4(normalize(cross(ab, ac)),1);
		vec4 tangent   = vec4(normalize(ab),1);
		vec4 bitangent = vec4(normalize(ac),1);

		// Emit the vertices with the calculated normal.
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i];
			outNormal = normal;
			outTangent = tangent;
			outBitangent = bitangent;
			outTextureCoordinate = geometryTextureCoordinate[i];
			EmitVertex();
		}
		EndPrimitive();
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

	instance->shader = VuoShader_make("Facet Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                 NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource,          geometryShaderSource, NULL);

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
		VuoOutputData(VuoSceneObject) facetedObject
)
{
	*facetedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
