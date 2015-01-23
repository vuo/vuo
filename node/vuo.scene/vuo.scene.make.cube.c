/**
 * @file
 * vuo.scene.make.cube node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Make Cube",
					 "keywords" : [ "box", "d6", "hexahedron", "Platonic", "rectangular", "square" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoShader) frontShader,
		VuoInputData(VuoShader) leftShader,
		VuoInputData(VuoShader) rightShader,
		VuoInputData(VuoShader) backShader,
		VuoInputData(VuoShader) topShader,
		VuoInputData(VuoShader) bottomShader,
		VuoOutputData(VuoSceneObject) cube
)
{
	*cube = VuoSceneObject_makeCube(transform, frontShader, leftShader, rightShader, backShader, topShader, bottomShader);
}
