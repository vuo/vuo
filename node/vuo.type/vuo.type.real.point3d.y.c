/**
 * @file
 * vuo.type.real.point3d node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to 3D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) y,
		VuoOutputData(VuoPoint3d) point3d
)
{
	*point3d = VuoPoint3d_make(0, y, 0);
}
