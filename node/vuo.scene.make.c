/**
 * @file
 * vuo.scene.make node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Object",
					 "description" :
						 "<p>Creates a 3D object that can be added to a 3D scene.</p> \
						 <p><ul> \
						 <li>`vertices` — The vertices (points in 3D space) that define the shape of the object. \
						 These should use Vuo coordinates. \
						 For this node to create a 3D object, you must give it some vertices. </li> \
						 <li>`shader` — A shader that determines how the vertices will be drawn, such as lighting and color. \
						 If no shader is provided, this node uses a default shader that stretches a gradient-colored checkerboard \
						 across the vertices.</li> \
						 <li>`transform` — A transform that changes the 3D object's translation, rotation, or scale. \
						 It should use Vuo coordinates.</li> \
						 </ul></p> \
						 <p>In Vuo coordinates, (0,0,0) is the center of the scene. \
						 The scene has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. \
						 The scene's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. \
						 The scene's camera is at (0,0,1), with the z-coordinate increasing from back to front.</p>",
					 "keywords" : [ "mesh", "model", "vertices", "shader", "texture", "draw", "opengl", "scenegraph", "graphics" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoList_VuoVertices) vertices,
		VuoInputData(VuoShader) shader,
		VuoInputData(VuoTransform) transform,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_make(
				vertices,
				shader,
				transform,
				NULL
			);
}
