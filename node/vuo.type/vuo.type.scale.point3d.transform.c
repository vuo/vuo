/**
 * @file
 * vuo.type.scale.point3d.transform node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Point to Scale",
					 "keywords" : [ "matrix", "trs", "size", "angle", "axis", "grow", "shrink", "stretch" ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":1, "y":1, "z":1}}) scale,
		VuoOutputData(VuoTransform) transform
)
{
	*transform = VuoTransform_makeEuler( VuoPoint3d_make(0.,0.,0.), VuoPoint3d_make(0,0,0), scale );
}
