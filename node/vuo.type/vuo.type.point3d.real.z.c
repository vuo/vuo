/**
 * @file
 * vuo.type.point3d.real node implementation.
 *
 * @copyright Copyright © 2012–2022 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert 3D Point to Real (Z)",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoPoint3d, {"default":{"x":0, "y":0, "z":0}, "name":"(X,Y,Z)"}) xyz,
		VuoOutputData(VuoReal) z
)
{
	*z = xyz.z;
}
