/**
 * @file
 * vuo.scene.bend node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
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
					 "version" : "1.0.1",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "QuantizeObject.vuo" ]
					 }
				 });

static const char *pointLineVertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// \n#include "deform.glsl" — don't use deform.glsl, since its finite-difference method of calculating the normal/tangent/bitangent doesn't play well with quantization.

	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;

	attribute vec3 position;
	attribute vec3 normal;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	varying vec3 outPosition;
	varying vec3 outNormal;
	varying vec2 outTextureCoordinate;
	varying vec4 outVertexColor;

	// Inputs
	uniform vec3 stepSize;
	uniform vec3 center;

	void main()
	{
		// Transform into worldspace.
		vec3 positionInScene = (modelviewMatrix * vec4(position, 1.)).xyz;

		// Apply the deformation.
		vec3 centerOffset = mod(center - stepSize/2., stepSize);
		vec3 distanceFromCorner = mod(positionInScene - centerOffset, stepSize);
		vec3 deformedPosition = positionInScene - distanceFromCorner + stepSize/2.;

		// Transform back into modelspace.
		outPosition = (modelviewMatrixInverse * vec4(deformedPosition, 1.)).xyz;

		outNormal = normal;
		outTextureCoordinate = textureCoordinate;
		outVertexColor = vertexColor;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}

);

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform mat4 modelviewMatrix;
	uniform mat4 modelviewMatrixInverse;

	attribute vec3 position;
	attribute vec2 textureCoordinate;
	attribute vec4 vertexColor;

	// Inputs
	uniform vec3 stepSize;
	uniform vec3 center;

	// Outputs
	varying vec3 geometryPosition;
	varying vec2 geometryTextureCoordinate;
	varying vec4 geometryVertexColor;

	void main()
	{
		// Transform into worldspace.
		vec3 positionInScene = (modelviewMatrix * vec4(position, 1.)).xyz;

		// Apply the deformation.
		vec3 centerOffset = mod(center - stepSize/2., stepSize);
		vec3 distanceFromCorner = mod(positionInScene - centerOffset, stepSize);
		vec3 deformedPosition = positionInScene - distanceFromCorner + stepSize/2.;

		// Transform back into modelspace.
		geometryPosition = (modelviewMatrixInverse * vec4(deformedPosition, 1.)).xyz;

		geometryTextureCoordinate = textureCoordinate;
		geometryVertexColor = vertexColor;

		// Older systems (e.g., NVIDIA GeForce 9400M on Mac OS 10.6)
		// fall back to the software renderer if gl_Position is not initialized.
		gl_Position = vec4(0);
	}
);

// Same as vuo.scene.facet
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

	instance->shader = VuoShader_make("Quantize Object Shader");
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
		VuoInputData(VuoPoint3d, {"default":{"x":0.1,"y":0.1,"z":0.1}, "suggestedMin":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.01,"y":0.01,"z":0.01}}) stepSize,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0,"z":-1.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoOutputData(VuoSceneObject) quantizedObject
)
{
	VuoPoint3d nonzeroStepSize = VuoPoint3d_makeNonzero(stepSize);

	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "stepSize", nonzeroStepSize);
	VuoShader_setUniform_VuoPoint3d((*instance)->shader, "center", center);

	*quantizedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			for (int i = 0; i < *vertexCount; ++i)
			{
				// Transform into worldspace.
				VuoPoint3d positionInScene = VuoTransform_transformPoint(modelMatrix, VuoPoint3d_makeFromArray(&positions[i * 3]));

				// Apply the deformation.
				VuoPoint3d centerOffset       = VuoPoint3d_mod(center - nonzeroStepSize / (VuoPoint3d)(2), nonzeroStepSize);
				VuoPoint3d distanceFromCorner = VuoPoint3d_mod(positionInScene - centerOffset, nonzeroStepSize);
				VuoPoint3d deformedPosition   = positionInScene - distanceFromCorner + nonzeroStepSize / (VuoPoint3d)(2);

				// Transform back into modelspace.
				VuoPoint3d_setArray(&positions[i * 3], VuoTransform_transformPoint(modelMatrixInverse, deformedPosition));
			}

			if (*vertexCount != 3)
				return;

			// Calculate two vectors in the plane of the input triangle.
			VuoPoint3d ab = VuoPoint3d_makeFromArray(&positions[1 * 3]) - VuoPoint3d_makeFromArray(&positions[0 * 3]);
			VuoPoint3d ac = VuoPoint3d_makeFromArray(&positions[2 * 3]) - VuoPoint3d_makeFromArray(&positions[0 * 3]);
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
