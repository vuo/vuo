/**
 * @file
 * vuo.type.real.point4d.xyzw node implementation.
 *
 * @copyright Copyright © 2012–2021 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see https://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Convert Real to 4D Point (X,X,X,X)",
					 "keywords" : [ ],
					 "version" : "1.0.0"
				 });

void nodeEvent
(
		VuoInputData(VuoReal, {"default":0.0}) x,
		VuoOutputData(VuoPoint4d, {"name":"(X,X,X,X)"}) point4d
)
{
	*point4d = VuoPoint4d_make(x, x, x, x);
}
