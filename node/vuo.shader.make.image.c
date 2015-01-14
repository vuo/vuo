/**
 * @file
 * vuo.shader.make.image node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoGlContext.h"

/// @todo After we drop 10.6 support, switch back to gl3.h.
//#include <OpenGL/gl3.h>
#include <OpenGL/gl.h>

VuoModuleMetadata({
					 "title" : "Shade with Image",
					 "description" :
						 "<p>Creates a graphics shader that can paint an image on a 3D object.</p> \
						 <p>The image is stretched across the vertices of the 3D object. \
						 Unless the vertices form a rectangle whose aspect ratio matches the image's, the image will be deformed.</p> \
						 <p>This shader ignores lighting.</p>",
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
		VuoShader_addTexture(*shader, image, "texture");
}
