/**
 * @file
 * vuo.type.translate.point3d.transform node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Point to Translation",
					 "keywords" : [ "position", "matrix", "trs", "angle", "axis" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}}) translation,
		VuoOutputData(VuoTransform) transform
)
{
	*transform = VuoTransform_makeEuler( translation, VuoPoint3d_make(0.,0.,0.), VuoPoint3d_make(1.,1.,1.) );
}
