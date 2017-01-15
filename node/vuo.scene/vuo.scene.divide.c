/**
 * @file
 * vuo.scene.divide node implementation.
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
					 "title" : "Divide 3D Object",
					 "keywords" : [ "break", "separate", "face", "edge", "side", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "DivideSphere.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	include(deform)

	// Inputs
	uniform float dist;

	vec3 deform(vec3 position)
	{
		// Simplified, origin-based version for point and line meshes, since they don't have normals.
		float l = length(position);
		return position + dist * l * l * sign(position);
	}
);

static const char *triangleVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	// Outputs
	varying vec4 geometryPosition;
	varying vec4 geometryNormal;
	varying vec4 geometryTangent;
	varying vec4 geometryBitangent;
	varying vec4 geometryTextureCoordinate;

	void main()
	{
		geometryPosition = position;
		geometryNormal = normal;
		geometryTangent = tangent;
		geometryBitangent = bitangent;
		geometryTextureCoordinate = textureCoordinate;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

static const char *triangleGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec4 geometryPosition[3];
	varying in vec4 geometryNormal[3];
	varying in vec4 geometryTangent[3];
	varying in vec4 geometryBitangent[3];
	varying in vec4 geometryTextureCoordinate[3];
	uniform float dist;

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

		// Emit the vertices with the calculated normal.
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i] + vec4(normal.xyz*dist,0);
			outNormal = geometryNormal[i];
			outTangent = geometryTangent[i];
			outBitangent = geometryBitangent[i];
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

	instance->shader = VuoShader_make("Divide Object");
	VuoShader_addSource(instance->shader, VuoMesh_Points,              pointLineVertexShaderSource, NULL,                         NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     pointLineVertexShaderSource, NULL,                         NULL);
	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, triangleVertexShaderSource,  triangleGeometryShaderSource, NULL);
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
		VuoInputData(VuoReal, {"default":0.1,"suggestedMin":0.0,"suggestedMax":1.0}) distance,
		VuoOutputData(VuoSceneObject) dividedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "dist", distance);

	// Render.
	*dividedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
