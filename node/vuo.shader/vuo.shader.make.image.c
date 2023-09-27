/**
 * @file
 * vuo.shader.make.image node implementation.
 *
 * @copyright Copyright © 2012–2023 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Lit Image Shader",
					 "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						 "lighting", "lit", "lighted",
						 "Blinn", "Phong", "Lambert" ],
					 "version" : "3.0.0",
					 "dependencies" : [
					 ],
					 "node" : {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) opacity,
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) highlightColor,	// Keep in sync with VuoShader_make_VuoImage.
		VuoInputData(VuoReal,{"default":0.9, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) shininess,	// Keep in sync with VuoShader_make_VuoImage.
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeLitImageShader(image, opacity, highlightColor, shininess);
}
