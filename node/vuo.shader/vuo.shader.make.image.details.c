/**
 * @file
 * vuo.shader.make.image.details node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include <OpenGL/CGLMacro.h>

VuoModuleMetadata({
					  "title" : "Shade with Details",
					  "keywords" : [ "texture", "paint", "draw", "opengl", "glsl", "scenegraph", "graphics",
						  "lighting", "lit", "lighted",
						  "Blinn", "Phong", "Lambert", "normal", "bump", "specular", "map" ],
					  "version" : "2.0.0",
					  "dependencies" : [
						  "VuoGlContext"
					  ],
					  "node": {
						  "exampleCompositions" : [ "MoveLightAcrossTile.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":1.0,"suggestedMin":0,"suggestedMax":1}) opacity,
		VuoInputData(VuoImage) specularImage,
		VuoInputData(VuoImage) normalImage,
		VuoOutputData(VuoShader) shader
)
{
	*shader = VuoShader_makeLitImageDetailsShader(image, opacity, specularImage, normalImage);
}
