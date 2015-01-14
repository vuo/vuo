/**
 * @file
 * vuo.shader.make.color node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

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


static const char * vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;
	attribute vec4 normal;

	// Outputs to fragment shader
	varying vec4 vertexNormal;

	void main()
	{
		vertexNormal = vec4(normalize(mat3(modelviewMatrix) * vec3(normal)), 1.);

		gl_Position = projectionMatrix * modelviewMatrix * position;
	}
);

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	// Inputs
	varying vec4 vertexNormal;

	void main()
	{
		gl_FragColor = abs(vertexNormal);
	}
);

void nodeEvent
(
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("normal colors shader", vertexShaderSource, fragmentShaderSource);
}
