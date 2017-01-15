/**
 * @file
 * vuo.shader.make.image.unlit node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Unlit Image",
					 "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics" ],
					 "version" : "3.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) opacity,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeUnlitImageShader(image, opacity);
}
