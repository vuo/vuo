/**
 * @file
 * vuo.type.integer.point2d node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 2D Point (0,Y)",
					 "keywords" : [ ],
					 "version" : "1.0.1"
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) y,
		VuoOutputData(VuoPoint2d, {"name":"(0,Y)"}) point2d
)
{
	*point2d = VuoPoint2d_make(0, y);
}
