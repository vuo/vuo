/**
 * @file
 * vuo.scene.make.image node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make 3D Object from Image",
					 "description" :
						 "<p>Turns an image into a 3D object that can be added to a 3D scene. \
						 The 3D object is like a piece of paper that displays the image on one side.</p> \
						 <p><ul> \
						 <li>`center` — The center point of the 3D object, in Vuo coordinates.</li> \
						 <li>`rotation` — The rotation of the 3D object, in degrees (Euler angles). \
						 At (0,0,0), the image is facing front.</li> \
						 <li>`width` — The width of the 3D object, in Vuo coordinates. \
						 The height is calculated from the width to preserve the image's aspect ratio.</li> \
						 </ul></p> \
						 <p>In Vuo coordinates, (0,0,0) is the center of the scene. \
						 The scene has a width of 2, with x-coordinate -1 on the left edge and 1 on the right edge. \
						 The scene's height is determined by its aspect ratio, with the y-coordinate increasing from bottom to top. \
						 The scene's camera is at (0,0,1), with the z-coordinate increasing from back to front.</p>",
					 "keywords" : [ "billboard", "sprite", "projector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) center,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) rotation,
		VuoInputData(VuoReal, {"default":1}) width,
		VuoOutputData(VuoSceneObject) object
)
{
	if (!image)
	{
		*object = VuoSceneObject_makeEmpty();
		return;
	}

	VuoList_VuoVertices verticesList = VuoListCreate_VuoVertices();
	VuoListAppendValue_VuoVertices(verticesList, VuoVertices_getQuad());
	*object = VuoSceneObject_make(
				verticesList,
				VuoShader_makeImageShader(),
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,image->pixelsHigh * width/image->pixelsWide,1)
				),
				NULL
			);
	VuoShader_addTexture(object->shader, image, "texture");
}
