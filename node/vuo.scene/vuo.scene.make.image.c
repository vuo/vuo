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
					 "keywords" : [ "billboard", "sprite", "projector" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "FlipCoin.vuo", "RippleImageOfSphere.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) center,
		VuoInputData(VuoPoint3d, {"default":{"x":0,"y":0,"z":0}}) rotation,
		VuoInputData(VuoReal, {"default":1.0}) width,
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
