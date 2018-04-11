/**
 * @file
 * vuo.scene.bend node implementation.
 *
 * @copyright Copyright © 2012–2017 Kosada Incorporated.
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
					 "title" : "Quantize 3D Object",
					 "keywords" : [
						 "filter",
						 "pixels", "lofi", "simplify", "block", "cube", "square", "mosaic",
						 "pixellate",
						 "pixelate", // American spelling
					 ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "QuantizeObject.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// include(deform) — don't use deform.glsl, since its finite-difference method of calculating the normal/tangent/bitangent doesn't play well with quantization.

	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;

	attribute vec4 position;
	attribute vec4 normal;
	attribute vec4 tangent;
	attribute vec4 bitangent;
	attribute vec4 textureCoordinate;

	varying vec4 outPosition;
	varying vec4 outNormal;
	varying vec4 outTangent;
	varying vec4 outBitangent;
	varying vec4 outTextureCoordinate;

	// Inputs
	uniform vec3 stepSize;
	uniform vec3 center;

	void main()
	{
		// Transform into worldspace.
		vec4 positionInScene = modelviewMatrix * position;

		// Apply the deformation.
		vec3 centerOffset = mod(center - stepSize/2., stepSize);
		vec3 distanceFromCorner = mod(position.xyz - centerOffset, stepSize);
		vec3 deformedPosition = position.xyz - distanceFromCorner + stepSize/2.;

		// Transform back into modelspace.
		outPosition = modelviewMatrixInverse * vec4(deformedPosition, 1.);

		outNormal    = normal;
		outTangent   = tangent;
		outBitangent = bitangent;
		outTextureCoordinate = textureCoordinate;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}

);

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;

	attribute vec4 position;
	attribute vec4 textureCoordinate;

	// Inputs
	uniform vec3 stepSize;
	uniform vec3 center;

	// Outputs
	varying vec4 geometryPosition;
	varying vec4 geometryTextureCoordinate;

	void main()
	{
		// Transform into worldspace.
		vec4 positionInScene = modelviewMatrix * position;

		// Apply the deformation.
		vec3 centerOffset = mod(center - stepSize/2., stepSize);
		vec3 distanceFromCorner = mod(position.xyz - centerOffset, stepSize);
		vec3 deformedPosition = position.xyz - distanceFromCorner + stepSize/2.;

		// Transform back into modelspace.
		geometryPosition = modelviewMatrixInverse * vec4(deformedPosition, 1.);

		geometryTextureCoordinate = textureCoordinate;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

// Same as vuo.scene.facet
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

	instance->shader = VuoShader_make("Quantize Object Shader");
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
		VuoInputData(VuoPoint3d, {"default":{"x":0.1,"y":0.1,"z":0.1}, "suggestedMin":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.01,"y":0.01,"z":0.01}}) stepSize,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0,"z":-1.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoOutputData(VuoSceneObject) quantizedObject
)
{
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "stepSize", VuoPoint3d_makeNonzero(stepSize));
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center",   center);
	*quantizedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
