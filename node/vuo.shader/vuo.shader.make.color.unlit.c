/**
 * @file
 * vuo.shader.make.color.unlit node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Unlit Color",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics", "solid", "self illumination", "tone", "chroma" ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeUnlitColorShader(color);

	{
		CGLContextObj cgl_ctx = (CGLContextObj)VuoGlContext_use();

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		glFlushRenderAPPLE();

		VuoGlContext_disuse(cgl_ctx);
	}
}
