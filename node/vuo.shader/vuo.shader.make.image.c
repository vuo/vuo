/**
 * @file
 * vuo.shader.make.image node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Shade with Image",
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
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeImageShader();

	if (image)
	{
		VuoGlContext glContext = VuoGlContext_use();

		VuoShader_addTexture(*shader, glContext, "texture", image);

		VuoGlContext_disuse(glContext);
	}
}
