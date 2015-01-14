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

/// @todo After we drop 10.6 support, switch back to gl3.h.
//#include <OpenGL/gl3.h>
#include <OpenGL/gl.h>

VuoModuleMetadata({
					 "title" : "Shade with Solid Color",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });


static const char * vertexShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform mat4 projectionMatrix;
	uniform mat4 modelviewMatrix;
	attribute vec4 position;

	void main()
	{
		gl_Position = projectionMatrix * modelviewMatrix * position;
	}
);

static const char * fragmentShaderSource = VUOSHADER_GLSL_SOURCE(120,
	uniform vec4 color;

	void main()
	{
		gl_FragColor = color;
	}
);

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_make("solid color shader", vertexShaderSource, fragmentShaderSource);

	VuoGlContext_use();
	{
		glUseProgram((*shader)->glProgramName);
		{
			GLint colorUniform = glGetUniformLocation((*shader)->glProgramName, "color");
			glUniform4f(colorUniform, color.r, color.g, color.b, color.a);
		}
		glUseProgram(0);
	}
	VuoGlContext_disuse();
}
