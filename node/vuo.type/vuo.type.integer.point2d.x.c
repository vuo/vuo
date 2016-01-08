/**
 * @file
 * vuo.type.integer.point2d node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 2D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) x,
		VuoOutputData(VuoPoint2d) point2d
)
{
	*point2d = VuoPoint2d_make(x, 0);
}
