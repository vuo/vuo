/**
 * @file
 * vuo.type.integer.point3d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 3D Point (0,Y,0)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) y,
		VuoOutputData(VuoPoint3d, {"name":"(0,Y,0)"}) point3d
)
{
	*point3d = VuoPoint3d_make(0, y, 0);
}
