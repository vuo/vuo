/**
 * @file
 * vuo.shader.make.color.unlit node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Make Unlit Color Shader",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics", "solid", "self illumination", "tone", "chroma" ],
					 "version" : "2.0.0",
					 "dependencies" : [
					 ],
					 "node": {
						  "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1,"g":1,"b":1,"a":1}}) color,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeUnlitColorShader(color);
}
