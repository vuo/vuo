/**
 * @file
 * vuo.shader.make.color node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					 "title" : "Shade with Color",
					 "keywords" : [ "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						 "lighting", "lit", "lighted",
						 "Blinn", "Phong", "Lambert", "tone", "chroma" ],
					 "version" : "2.0.0",
					 "dependencies" : [
						 "VuoGlContext"
					 ],
					 "node" : {
						  "exampleCompositions" : [ "CompareColorAndEdgeShaders.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoColor,{"default":{"r":1.,"g":0.8,"b":0.6,"a":1}}) color,
		VuoInputData(VuoColor,{"default":{"r":1.,"g":1.,"b":1.,"a":1.}}) highlightColor,	// Keep in sync with VuoShader_make_VuoImage.
		VuoInputData(VuoReal,{"default":0.9, "suggestedMin":0.0, "suggestedMax":1.0, "suggestedStep":0.1}) shininess,	// Keep in sync with VuoShader_make_VuoImage.
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeLitColorShader(color, highlightColor, shininess);
}
