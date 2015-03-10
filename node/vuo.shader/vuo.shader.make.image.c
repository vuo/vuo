/**
 * @file
 * vuo.shader.make.image node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Image",
					 "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics", "lighting", "Blinn", "Phong", "Lambert" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) alpha,
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) highlightColor,
		VuoInputData(VuoReal,{"default":0.9, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) shininess,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeLitImageShader(image, alpha, highlightColor, shininess);
}
