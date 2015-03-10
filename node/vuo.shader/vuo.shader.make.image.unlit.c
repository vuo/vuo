/**
 * @file
 * vuo.shader.make.image.unlit node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Unlit Image",
					 "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics" ],
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
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) alpha,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeImageShader();

	if (image)
	{
		VuoGlContext glContext = VuoGlContext_use();

		VuoShader_addTexture(*shader, glContext, "texture", image);

		VuoShader_setUniformFloat(*shader, glContext, "alpha", alpha);

		// Ensure the command queue gets executed before we return,
		// since the VuoShader might immediately be used on another context.
		{
			CGLContextObj cgl_ctx = (CGLContextObj)glContext;
			glFlushRenderAPPLE();
		}

		VuoGlContext_disuse(glContext);
	}
}
