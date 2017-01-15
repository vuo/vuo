/**
 * @file
 * vuo.scene.back node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include <OpenGL/gl.h>

VuoModuleMetadata({
					  "title" : "Show Back of 3D Object",
					  "keywords" : [ "interior", "inside", "behind", "backface", "culling", "disappear", "transparent", "visible", "visibility",
						  "two-sided", "two", "sided", "twosided", "double-sided", "double", "doublesided", "2-sided", "2",
						  "face", "facing", "away", "normal"],
					  "version" : "1.0.0",
					  "node": {
						  "exampleCompositions" : [ "ExplodeSphere.vuo", "TwirlGrid.vuo" ]
					  }
				 });

void nodeEvent
(
	VuoInputData(VuoSceneObject) object,
	VuoInputData(VuoBoolean, {"default": true}) keepFront,
	VuoOutputData(VuoSceneObject) backObject
)
{
	*backObject = VuoSceneObject_copy(object);
	unsigned int faceCullingMode = keepFront ? GL_NONE : GL_FRONT;
	VuoSceneObject_setFaceCullingMode(backObject, faceCullingMode);
}
