/**
 * @file
 * vuo.shader.make.color node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Normal Colors",
					 "keywords" : [ "mesh", "draw", "opengl", "glsl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "dependencies" : [
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });


static const char *vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 normal;

	// Outputs to fragment shader
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		vertexPlaneToWorld[2] = normalize(vec3(modelviewMatrix * normal));

		gl_Position = projectionMatrix * modelviewMatrix * position;
	}
);

static const char *vertexShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs provided by VuoSceneRenderer
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;

	// Outputs to geometry shader
	varying vec4 positionForGeometry;

	void main()
	{
		positionForGeometry = modelviewMatrix * position;
		gl_Position = projectionMatrix * positionForGeometry;
	}
);

const char *pointGeometryShaderSource = VUOSHADER_GLSL_SOURCE(120, include(trianglePoint));
const char *lineGeometryShaderSource  = VUOSHADER_GLSL_SOURCE(120, include(triangleLine));

static const char *fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from vertex/geometry shader
	varying mat3 vertexPlaneToWorld;

	void main()
	{
		vec3 normal = vertexPlaneToWorld[2];

		gl_FragColor = vec4(abs(normal), 1);
	}
);

static const char *fragmentShaderSourceForGeometry = VUOSHADER_GLSL_SOURCE(120,
	// Inputs from vertex/geometry shader
	varying vec4 vertexPosition;
	varying mat3 vertexPlaneToWorld;
	varying vec4 fragmentTextureCoordinate;

	void main()
	{
		vertexPosition;
		fragmentTextureCoordinate;

		vec3 normal = vertexPlaneToWorld[2];

		gl_FragColor = vec4(abs(normal), 1);
	}
);


void nodeEvent
(
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("Normal Colors Shader");

	VuoShader_addSource                      (*shader, VuoMesh_Points,              vertexShaderSourceForGeometry, pointGeometryShaderSource, fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_Points, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualLines,     vertexShaderSourceForGeometry, lineGeometryShaderSource,  fragmentShaderSourceForGeometry);
	VuoShader_setExpectedOutputPrimitiveCount(*shader, VuoMesh_IndividualLines, 2);

	VuoShader_addSource                      (*shader, VuoMesh_IndividualTriangles, vertexShaderSource,            NULL,                      fragmentShaderSource);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}
}
