/**
 * @file
 * vuo.shader.make.image.unlit node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Unlit Image Shader",
					 "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics" ],
					 "version" : "3.0.0",
					 "dependencies" : [
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
