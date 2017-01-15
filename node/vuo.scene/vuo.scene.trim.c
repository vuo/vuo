/**
 * @file
 * vuo.scene.trim node implementation.
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
					 "title" : "Trim 3D Object",
					 "keywords" : [ "resize", "snip", "clip", "sample", "rectangle", "box", "cube", "crop", "cull", "prune", "crosssection", "cross-section", "cross", "section", "filter" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext",
						 "VuoSceneObjectRenderer"
					 ],
					 "node": {
						 "exampleCompositions" : [ "TrimSphere.vuo" ]
					 }
				 });

static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
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

static const char *geometryShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying in vec4 geometryPosition[3];
	varying in vec4 geometryNormal[3];
	varying in vec4 geometryTangent[3];
	varying in vec4 geometryBitangent[3];
	varying in vec4 geometryTextureCoordinate[3];
	uniform mat4 modelviewMatrix;
	uniform float left;
	uniform float right;
	uniform float top;
	uniform float bottom;
	uniform float front;
	uniform float back;

	// Outputs
	varying out vec4 outPosition;
	varying out vec4 outNormal;
	varying out vec4 outTangent;
	varying out vec4 outBitangent;
	varying out vec4 outTextureCoordinate;

	void main()
	{
		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			// Transform into worldspace.
			vec4 positionInScene = modelviewMatrix * geometryPosition[i];

			if (positionInScene.x < left  || positionInScene.x > right
			 || positionInScene.y > top   || positionInScene.y < bottom
			 || positionInScene.z > front || positionInScene.z < back)
				return;
		}

		for (int i = 0; i < gl_VerticesIn; ++i)
		{
			outPosition = geometryPosition[i];
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

	instance->shader = VuoShader_make("Trim Object");

	VuoShader_addSource(instance->shader, VuoMesh_Points,              vertexShaderSource, geometryShaderSource, NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualLines,     vertexShaderSource, geometryShaderSource, NULL);

	VuoShader_addSource(instance->shader, VuoMesh_IndividualTriangles, vertexShaderSource, geometryShaderSource, NULL);
	VuoShader_setMayChangeOutputPrimitiveCount(instance->shader, VuoMesh_IndividualTriangles, true);

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
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0},"suggestedStep":{"x":0.1,"y":0.1,"z":0.1}}) center,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) width,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) height,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0.0,"suggestedStep":0.1}) depth,
		VuoOutputData(VuoSceneObject) trimmedObject
)
{
	// Feed parameters to the shader.
	VuoShader_setUniform_VuoReal((*instance)->shader, "left",   center.x-width/2);
	VuoShader_setUniform_VuoReal((*instance)->shader, "right",  center.x+width/2);
	VuoShader_setUniform_VuoReal((*instance)->shader, "top",    center.y+height/2);
	VuoShader_setUniform_VuoReal((*instance)->shader, "bottom", center.y-height/2);
	VuoShader_setUniform_VuoReal((*instance)->shader, "front",  center.z+depth/2);
	VuoShader_setUniform_VuoReal((*instance)->shader, "back",   center.z-depth/2);

	// Render.
	*trimmedObject = VuoSceneObjectRenderer_draw((*instance)->sceneObjectRenderer, object);
}

void nodeInstanceFini(VuoInstanceData(struct nodeInstanceData *) instance)
{
	VuoRelease((*instance)->shader);
	VuoRelease((*instance)->sceneObjectRenderer);
	VuoGlContext_disuse((*instance)->glContext);
}
