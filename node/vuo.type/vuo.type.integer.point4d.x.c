/**
 * @file
 * vuo.type.integer.point4d node implementation.
 *
 * @copyright Copyright © 2012–2015 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Integer to 4D Point",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoInteger, {"default":0}) x,
		VuoOutputData(VuoPoint4d) point4d
)
{
	*point4d = VuoPoint4d_make(x, 0, 0, 0);
}
