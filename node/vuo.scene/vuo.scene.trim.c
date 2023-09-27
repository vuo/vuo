/**
 * @file
 * vuo.scene.trim node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "VuoSceneObjectRenderer.h"

#include "VuoGlPool.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Trim 3D Object",
					 "keywords" : [
						  "resize", "snip", "clip", "sample", "rectangle", "box", "cube", "crop", "cull", "prune",
						  "crosssection", "cross-section", "cross", "section", "filter", "size", "dimensions"
					  ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "TrimSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
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

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec3 geometryPosition[3];
	varying in vec3 geometryNormal[3];
	varying in vec2 geometryTextureCoordinate[3];
	varying in vec4 geometryVertexColor[3];
	uniform mat4 modelviewMatrix;
	uniform float left;
	uniform float right;
	uniform float top;
	uniform float bottom;
	uniform float front;
	uniform float back;

	// Outputs
	varying out vec3 outPosition;
	varying out vec3 outNormal;
	varying out vec2 outTextureCoordinate;
	varying out vec4 outVertexColor;

	void main()
	{
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			// Transform into worldspace.
			vec3 positionInScene = (modelviewMatrix * vec4(geometryPosition[i], 1.)).xyz;

			if (positionInScene.x < left  || positionInScene.x > right
			 || positionInScene.y > top   || positionInScene.y < bottom
			 || positionInScene.z > front || positionInScene.z < back)
				return;
		}

		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i];
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

	instance->shader = VuoShader_make("Trim Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              vertexShaderSource, geometryShaderSource, NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     vertexShaderSource, geometryShaderSource, NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource, geometryShaderSource, NULL);
	VuoShader_setMayChangeOutputPrimitiveCount(instance->shader, VuoMesh_IndividualTriangles, true);

	VuoRetain(instance->shader);

	instance->sceneObjectRenderer = VuoSceneObjectRenderer_make(instance->shader);
	VuoRetain(instance->sceneObjectRenderer);

	return instance;
}

void nodeInstanceEvent
(
		VuoInstanceData(struct nodeInstanceData *) instance,
		VuoInputData(VuoSceneObject) object,
		VuoInputData(VuoPoint3d, {"default":{"x":0.0,"y":0.0,"z":0.0}, "suggestedMin":{"x":-1.0,"y":-1.0,"z":-1.0}, "suggestedMax":{"x":1.0,"y":1.0,"z":1.0}, "suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) height,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) depth,
		VuoOutputData(VuoSceneObject) trimmedObject
)
{
	double left   = center.x - width  / 2;
	double right  = center.x + width  / 2;
	double top    = center.y + height / 2;
	double bottom = center.y - height / 2;
	double front  = center.z + depth  / 2;
	double back   = center.z - depth  / 2;

	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "left",   left);
	VuoShader_setUniform_VuoReal((*instance)->shader, "right",  right);
	VuoShader_setUniform_VuoReal((*instance)->shader, "top",    top);
	VuoShader_setUniform_VuoReal((*instance)->shader, "bottom", bottom);
	VuoShader_setUniform_VuoReal((*instance)->shader, "front",  front);
	VuoShader_setUniform_VuoReal((*instance)->shader, "back",   back);

	// Render.
	*trimmedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object,
		^(float *modelMatrix, float *modelMatrixInverse, int *vertexCount, float *positions, float *normals, float *textureCoordinates, float *colors) {
			for (int i = 0; i < *vertexCount; ++i)
			{
				// Transform into worldspace.
				VuoPoint3d positionInScene = VuoTransform_transformPoint(modelMatrix, VuoPoint3d_makeFromArray(&positions[i * 3]));

				if (positionInScene.x < left  || positionInScene.x > right
				 || positionInScene.y > top   || positionInScene.y < bottom
				 || positionInScene.z > front || positionInScene.z < back)
				{
					*vertexCount = 0;
					return;
				}
			}
		});
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
}
